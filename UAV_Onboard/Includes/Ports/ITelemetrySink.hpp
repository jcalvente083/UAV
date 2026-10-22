/**
* @file ITelemetrySink.hpp
*
* @brief Puerto para el envío de la telemetría del dron.
*
* @details Define la interfaz que deben implementar los transmisores de
*          telemetría. Hay tres implementaciones: UDPTelemetrySink,
*          TCPTelemetrySink y MQTTTelemetrySink. main.cpp elige cuál instanciar
*          según el campo transporte de config.json.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include <string>

/**
* @class ITelemetrySink
*
* @brief Puerto (arquitectura hexagonal) para la transmisión de telemetría.
*
* @details Es el canal conmutable del sistema: el mismo JSON puede viajar por
*          UDP, TCP o MQTT sin que EdgeStreamingService sepa cuál se está
*          usando. El sink recibe el mensaje ya serializado (ver
*          TelemetrySerializer), de modo que el transporte no conoce el
*          esquema de los datos, solo envía bytes.
*
* @note El framing de cada mensaje depende del transporte, según el contrato
*       de protocolo:
*       - UDP: un datagrama = un objeto JSON.
*       - TCP: prefijo de 4 bytes con la longitud en big-endian, seguido del JSON.
*       - MQTT: un mensaje publicado en el topic configurado (por defecto
*         "uav/telemetry"), con QoS 1.
*/
class ITelemetrySink {
public:
    /**
    * @brief Destructor virtual.
    *
    * @details Garantiza que se llame al destructor del adaptador concreto al
    *          destruirlo a través de un puntero a ITelemetrySink.
    */
    virtual ~ITelemetrySink() = default;

    /**
    * @brief Prepara el canal de envío.
    *
    * @details El significado de host y port, y lo que se comprueba, depende
    *          de la implementación:
    *          - UDP: IP y puerto de la estación de tierra. No hay conexión real,
    *            así que devuelve true aunque nadie esté escuchando todavía.
    *          - TCP: IP y puerto de la estación de tierra, con handshake real.
    *            La estación debe estar ya escuchando cuando se llama a start():
    *            se hace un único intento de conexión.
    *          - MQTT: dirección y puerto del BROKER (típicamente 1883), no de la
    *            estación de tierra, que se conecta al mismo broker por su cuenta.
    *
    * @param[in] host Dirección de destino según el transporte (ver arriba). Dentro
    *            de Docker no sirve "127.0.0.1": hay que usar la IP real del equipo.
    * @param[in] port Puerto de destino según el transporte.
    *
    * @retval true si el canal quedó listo para publicar.
    * @retval false si no se pudo crear el socket o conectar (en TCP, por ejemplo,
    *         porque la estación de tierra aún no estaba escuchando).
    */
    virtual bool start(const std::string& host, int port) = 0;

    /**
    * @brief Envía un mensaje de telemetría ya serializado.
    *
    * @param[in] json Mensaje JSON completo, tal como lo genera TelemetrySerializer.
    *            El framing propio del transporte lo añade la implementación.
    *
    * @retval true si el mensaje se envió o se entregó a la librería de transporte.
    * @retval false si el canal no está iniciado o falló el envío. No es fatal:
    *         la telemetría es "último estado conocido" y el siguiente paquete
    *         la sustituye.
    */
    virtual bool publish(const std::string& json) = 0;

    /**
    * @brief Cierra el canal y libera los recursos de red.
    *
    * @details Debe poder llamarse de forma segura más de una vez (las
    *          implementaciones también lo invocan desde su destructor).
    */
    virtual void stop() = 0;
};