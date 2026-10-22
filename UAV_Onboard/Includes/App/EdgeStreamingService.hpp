/**
* @file EdgeStreamingService.hpp
*
* @brief Servicio de aplicación que orquesta el bucle principal del dron.
*
* @details Une todos los puertos del sistema embarcado: captura de vídeo,
*          detección, telemetría y envío de ambos canales a la estación de tierra.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include <atomic>
#include <vector>
#include "Ports/IVideoSource.hpp"
#include "Ports/IDetector.hpp"
#include "Ports/ITelemetryProvider.hpp"
#include "Ports/ITelemetrySink.hpp"
#include "Ports/IVideoSink.hpp"
#include "App/Config.hpp"

/**
* @class EdgeStreamingService
*
* @brief Bucle principal del dron: capturar, detectar, publicar telemetría y publicar vídeo.
*
* @details Solo conoce los puertos (interfaces), nunca los adaptadores concretos:
*          main.cpp decide qué implementaciones usar y se las inyecta en el
*          constructor. Gracias a ello, cambiar de transporte, de modelo o de
*          fuente de vídeo no requiere tocar esta clase.
*
*          En cada vuelta del bucle:
*          1. Lee un frame de IVideoSource.
*          2. Si ha pasado el intervalo mínimo desde la última inferencia, ejecuta
*             IDetector::detect() y guarda el resultado; si no, reutiliza las
*             últimas detecciones guardadas.
*          3. Toma una muestra de ITelemetryProvider, le añade las detecciones,
*             la serializa a JSON y la publica por ITelemetrySink.
*          4. Publica el frame por IVideoSink.
*          5. Duerme solo el tiempo que falte para el periodo objetivo
*             (1000 / fps ms). Si la vuelta ya tardó más que el periodo, no
*             duerme y no intenta recuperar el retraso: el bucle simplemente va
*             más lento, sin acumular deuda.
*
*          Todo se ejecuta en un único hilo. Es una decisión deliberada: el
*          sistema está pensado para medio núcleo de CPU y para un posible paso a
*          un RTOS sin soporte de múltiples hilos.
*
* @note Vídeo y detecciones no están sincronizados frame a frame. Las
*       detecciones que acompañan a cada paquete pueden proceder de un frame
*       anterior y repetirse en varios paquetes seguidos. Se aceptó este
*       desfase por simplicidad.
*
* @note Se publica un paquete de telemetría por cada vuelta del bucle, así que
*       la frecuencia de telemetría coincide con los fps efectivos del vídeo.
*
* @warning El intervalo mínimo entre inferencias (100 ms) solo es efectivo si
*          la inferencia dura menos que eso. Con la CPU restringida (por ejemplo,
*          Docker con --cpus=0.5) cada inferencia tarda varios cientos de ms, se
*          vuelve a inferir en todas las vueltas, el bucle va al ritmo de la
*          inferencia y el vídeo se congela durante cada una. La solución sería
*          inferir en un hilo aparte, descartada de momento por el objetivo RTOS.
*/
class EdgeStreamingService {
public:
    /**
    * @brief Construye el servicio con los adaptadores ya creados y configurados.
    *
    * @details Guarda REFERENCIAS a los adaptadores, no los posee: quien los crea
    *          (main.cpp) debe mantenerlos vivos mientras se ejecuta run(). La
    *          configuración sí se copia.
    *
    * @param[in] videoSource Fuente de vídeo ya abierta.
    * @param[in] detector Detector con el modelo ya cargado.
    * @param[in] telemetryProvider Proveedor de telemetría.
    * @param[in] telemetrySink Sink de telemetría ya iniciado (start() con éxito).
    * @param[in] videoSink Sink de vídeo ya iniciado (start() con éxito).
    * @param[in] config Configuración cargada; se usa videoTargetFps, que debe ser mayor que 0.
    */
    EdgeStreamingService(IVideoSource& videoSource,
        IDetector& detector,
        ITelemetryProvider& telemetryProvider,
        ITelemetrySink& telemetrySink,
        IVideoSink& videoSink,
        const Config& config);

    /**
    * @brief Ejecuta el bucle principal hasta que se pida parar.
    *
    * @details Bloquea al llamador hasta que stopFlag pase a true. Esa bandera la
    *          activa el manejador de SIGINT/SIGTERM de main.cpp (Ctrl+C o
    *          docker stop). La comprobación se hace al inicio de cada vuelta, así
    *          que el bucle termina tras completar la vuelta en curso, que puede
    *          incluir una inferencia.
    *
    *          Si no se puede leer un frame, avisa por consola, espera 50 ms y lo
    *          reintenta en la siguiente vuelta, sin salir del bucle. Con un
    *          fichero sin modo bucle, al terminar el vídeo se repetiría ese aviso
    *          hasta que se pare el programa.
    *
    *          La primera vuelta infiere siempre, para tener detecciones desde
    *          el primer paquete.
    *
    * @param[in] stopFlag Bandera de parada compartida con el manejador de señales.
    */
    void run(std::atomic<bool>& stopFlag);

private:
    IVideoSource& videoSource;              ///< Fuente de frames (no se posee).
    IDetector& detector;                    ///< Detector de objetos (no se posee).
    ITelemetryProvider& telemetryProvider;  ///< Proveedor de telemetría (no se posee).
    ITelemetrySink& telemetrySink;          ///< Canal de telemetría (no se posee).
    IVideoSink& videoSink;                  ///< Canal de vídeo (no se posee).
    Config config;                          ///< Copia de la configuración.

    std::vector<Detection> cachedDetections; ///< Últimas detecciones calculadas; se reutilizan en las vueltas en que no se infiere.
};