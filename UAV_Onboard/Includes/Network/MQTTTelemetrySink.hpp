/**
* @file MQTTTelemetrySink.hpp
*
* @brief Adaptador MQTT del puerto ITelemetrySink, basado en libmosquitto.
*
* @details Publica cada mensaje JSON de telemetría en un topic de un broker MQTT
*          (por ejemplo, Mosquitto en Docker). La estación de tierra no recibe
*          directamente del dron: se suscribe al mismo topic en el mismo broker
*          (MQTTReceiver), que es quien reparte los mensajes.
*
*          libmosquitto se obtiene en Windows a través de vcpkg (paquete
*          unofficial-mosquitto) y se usa la misma librería en el dron y en la
*          estación de tierra.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include <string>
#include <mosquitto.h>
#include "Ports/ITelemetrySink.hpp"

/**
* @class MQTTTelemetrySink
*
* @brief Adaptador (arquitectura hexagonal) que implementa ITelemetrySink sobre MQTT.
*
* @details La red la gestiona un hilo interno de libmosquitto, arrancado con
*          mosquitto_loop_start(). Gracias a ello:
*          - publish() no bloquea el bucle principal: solo entrega el mensaje a
*            la librería, que lo envía en segundo plano.
*          - Si se pierde la conexión con el broker, la propia librería intenta
*            reconectar automáticamente, sin intervención de este adaptador.
*
* @note Los mensajes se publican con QoS 1 ("al menos una vez"): el broker
*       confirma cada mensaje y libmosquitto lo reenvía si no recibe la
*       confirmación. Con broker caído, los mensajes pendientes pueden quedarse
*       encolados en memoria hasta que se recupere la conexión.
*/
class MQTTTelemetrySink : public ITelemetrySink {
public:
    /**
    * @brief Construye el sink e inicializa libmosquitto.
    *
    * @details Llama a mosquitto_lib_init(), necesario antes de usar cualquier
    *          otra función de la librería. Su pareja, mosquitto_lib_cleanup(),
    *          se llama en el destructor, por lo que solo debe existir una
    *          instancia de este sink a la vez en el proceso.
    *
    * @param[in] topic Topic en el que publicar (campo mqtt_topic de config.json;
    *            por defecto, "uav/telemetry", el definido en el contrato).
    */
    explicit MQTTTelemetrySink(std::string topic = "uav/telemetry");

    /**
    * @brief Destructor. Llama a stop() si sigue conectado y libera libmosquitto.
    */
    ~MQTTTelemetrySink() override;

    /**
    * @brief Conecta con el broker y arranca el hilo de red de libmosquitto.
    *
    * @details Crea el cliente con un identificador aleatorio y sesión limpia,
    *          conecta con un keepalive de 60 s y arranca el hilo interno. Si
    *          algún paso falla, deshace los anteriores antes de devolver false.
    *
    * @param[in] host Dirección del BROKER (no de la estación de tierra). Admite
    *            tanto IP como nombre de host, porque libmosquitto resuelve el
    *            nombre por sí misma.
    * @param[in] port Puerto del broker (normalmente 1883).
    *
    * @retval true si la conexión se estableció y el hilo de red quedó en marcha.
    * @retval false si no se pudo crear el cliente, conectar con el broker o
    *         arrancar el hilo de red. El fallo de conexión se informa por consola.
    */
    bool start(const std::string& host, int port) override;

    /**
    * @brief Publica un mensaje JSON en el topic configurado.
    *
    * @param[in] json Mensaje ya serializado; se envía tal cual como payload,
    *            sin framing adicional (MQTT ya delimita los mensajes).
    *
    * @retval true si libmosquitto aceptó el mensaje para su envío. No garantiza
    *         que ya haya llegado al broker: el envío real ocurre en el hilo de red.
    * @retval false si el sink no está iniciado o la librería rechazó el mensaje.
    */
    bool publish(const std::string& json) override;

    /**
    * @brief Detiene el hilo de red, se desconecta del broker y libera el cliente.
    *
    * @details Es seguro llamarlo varias veces: si el cliente ya fue liberado,
    *          no hace nada.
    */
    void stop() override;

private:
    struct mosquitto* mosq = nullptr;   ///< Cliente de libmosquitto; nullptr mientras no esté conectado.
    bool connected = false;             ///< true tras un start() con éxito.
    std::string topic;                  ///< Topic en el que se publica la telemetría.
};