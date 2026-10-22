/**
* @file main.cpp
*
* @brief Punto de entrada del sistema embarcado del dron (UAV_Onboard).
*
* @details Composition root de la aplicación: es el ÚNICO sitio que conoce los
*          adaptadores concretos (OnnxYoloAdapter, OpenCvVideoSource, los sinks
*          de red...). Los crea según config.json, los inyecta en
*          EdgeStreamingService a través de sus puertos y arranca el bucle
*          principal. El resto del sistema solo depende de interfaces.
*
*          Secuencia de arranque (cualquier fallo termina el programa con
*          código -1, antes de empezar a emitir nada):
*          1. Registrar los manejadores de SIGINT y SIGTERM.
*          2. Cargar config.json.
*          3. Cargar el modelo de detección.
*          4. Abrir la fuente de vídeo (en modo bucle).
*          5. Crear el proveedor de telemetría simulada.
*          6. Crear e iniciar el sink de telemetría según el transporte.
*          7. Crear e iniciar el sink de vídeo (siempre UDP).
*          8. Ejecutar el bucle hasta recibir una señal de parada.
*          9. Cerrar ambos sinks de forma ordenada.
*
* @warning El sink de vídeo usa siempre groundStationHost como destino. En modo
*          MQTT ese campo es la dirección del BROKER, no de la estación de
*          tierra, así que el vídeo solo llega si broker y estación están en la
*          misma máquina. Separar ambas direcciones en config.json queda como
*          mejora pendiente.
*
* @author Jesús David Calvente Zapata.
*/
#include <iostream>
#include <memory>
#include <atomic>
#include <csignal>

#include "App/Config.hpp"
#include "Ports/IDetector.hpp"
#include "Detection/OnnxYoloAdapter.hpp"
#include "Ports/IVideoSource.hpp"
#include "Video/OpenCvVideoSource.hpp"
#include "Ports/ITelemetryProvider.hpp"
#include "Telemetry/SimulatedTelemetryProvider.hpp"
#include "Ports/ITelemetrySink.hpp"
#include "Network/UDPTelemetrySink.hpp"
#include "Network/TCPTelemetrySink.hpp"
#include "Ports/IVideoSink.hpp"
#include "Network/UDPVideoSink.hpp"
#include "Network/MQTTTelemetrySink.hpp"
#include "App/EdgeStreamingService.hpp"

/**
* @brief Bandera global de parada, compartida entre el manejador de señales y el bucle principal.
*
* @details Tiene que ser global porque un manejador de señales es una función
*          C sin parámetros de contexto, y no tiene otra forma de comunicarse
*          con el resto del programa. Es std::atomic para que la escritura desde
*          el manejador y la lectura desde el bucle sean seguras.
*/
std::atomic<bool> stopFlag{ false };

/**
* @brief Manejador de SIGINT (Ctrl+C) y SIGTERM (docker stop).
*
* @details Solo activa stopFlag. No cierra nada directamente: el bucle de
*          EdgeStreamingService lo detecta al empezar la siguiente vuelta, sale
*          con normalidad y main() hace el apagado ordenado. Así el contenedor
*          Docker se detiene en uno o dos segundos en lugar de esperar al
*          SIGKILL forzado.
*
* @note Al ejecutar con el depurador de Visual Studio (F5), el propio depurador
*       puede interceptar Ctrl+C y la señal no llegar a este manejador. Dentro
*       de Docker (docker stop) funciona siempre.
*
* @param[in] signum Número de la señal recibida (no se usa).
*/
void signalHandler(int signum) {
    stopFlag = true;
}

/**
* @brief Función principal: monta el sistema y ejecuta el bucle hasta la señal de parada.
*
* @details Elección del transporte de telemetría según el campo transporte de
*          config.json: "tcp" crea TCPTelemetrySink, "mqtt" crea
*          MQTTTelemetrySink, y cualquier otro valor (incluido "udp" o un valor
*          mal escrito) crea UDPTelemetrySink, sin aviso.
*
*          Al terminar el bucle se llama explícitamente a stop() en ambos sinks.
*          Después, los std::unique_ptr destruyen los adaptadores en orden
*          inverso a su creación; los destructores de los sinks ya no vuelven a
*          llamar a stop(), porque su flag connected quedó a false.
*
* @retval 0 si el programa terminó por una señal de parada, tras el apagado limpio.
* @retval -1 si falló algún paso del arranque (modelo, vídeo o sinks).
*/
int main() {
    std::cout << "[INFO] Iniciando Sistema de Vuelo - Core IA..." << std::endl;

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    Config config = loadConfig("config.json");

    std::unique_ptr<IDetector> detector = std::make_unique<OnnxYoloAdapter>();
    if (!detector->loadModel(config.modelPath)) {
        std::cerr << "[FATAL] Fallo al cargar el modelo." << std::endl;
        return -1;
    }

    std::unique_ptr<IVideoSource> videoSource = std::make_unique<OpenCvVideoSource>(/*loop=*/true);
    if (!videoSource->open(config.videoSource)) {
        std::cerr << "[FATAL] No se pudo abrir la fuente de video." << std::endl;
        return -1;
    }

    std::unique_ptr<ITelemetryProvider> telemetryProvider =
        std::make_unique<SimulatedTelemetryProvider>(config.initialLatitude, config.initialLongitude);

    std::unique_ptr<ITelemetrySink> telemetrySink;
    if (config.transport == "tcp") {
        telemetrySink = std::make_unique<TCPTelemetrySink>();
    }
    else if (config.transport == "mqtt") {
        telemetrySink = std::make_unique<MQTTTelemetrySink>(config.mqttTopic);
    }
    else {
        telemetrySink = std::make_unique<UDPTelemetrySink>();
    }

    if (!telemetrySink->start(config.groundStationHost, config.telemetryPort)) {
        std::cerr << "[FATAL] No se pudo iniciar el sink de telemetria (" << config.transport << ")." << std::endl;
        return -1;
    }

    std::unique_ptr<IVideoSink> videoSink = std::make_unique<UDPVideoSink>();
    if (!videoSink->start(config.groundStationHost, config.videoPort)) {
        std::cerr << "[FATAL] No se pudo iniciar el sink de video." << std::endl;
        return -1;
    }

    std::cout << "[EXITO] Todo listo. Transporte: " << config.transport
        << " | Video: " << config.videoTargetFps << " fps" << std::endl;

    EdgeStreamingService service(*videoSource, *detector, *telemetryProvider,
        *telemetrySink, *videoSink, config);

    service.run(stopFlag);

    telemetrySink->stop();
    videoSink->stop();
    std::cout << "[INFO] Apagado limpio completado." << std::endl;

    return 0;
}