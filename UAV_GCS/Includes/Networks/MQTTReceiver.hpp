/**
* @file MQTTReceiver.hpp
*
* @brief Adaptador MQTT para el puerto IReceiver, usando libmosquitto.
*
* @details A diferencia de UDPReceiver/TCPReceiver (servidores pasivos que
*          escuchan en cualquier interfaz), MQTTReceiver actúa como cliente
*          activo: se conecta a un broker en una dirección concreta y se
*          suscribe a un topic, igual que MQTTTelemetrySink hace en el lado
*          del dron. Usa la misma librería C (libmosquitto) en ambos
*          proyectos, evitando depender del módulo QtMqtt (no disponible sin
*          compilarlo desde fuente en la versión open source de Qt).
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include "Ports/IReceiver.hpp"
#include <mosquitto.h>
#include <QByteArray>
#include <string>

/**
* @class MQTTReceiver
*
* @brief Adaptador (arquitectura hexagonal) que implementa IReceiver sobre MQTT.
*
* @details Usa mosquitto_loop_start() para que libmosquitto gestione la red
*          en un hilo propio, interno a la librería, sin necesidad de integrar
*          manualmente un bucle de red en el hilo de la GUI de Qt.
*
* @warning listen() aquí es distinto a UDPReceiver/TCPReceiver: mosquitto_connect()
*          y mosquitto_subscribe() son llamadas síncronas y bloqueantes, y
*          deviceConnected() se emite DENTRO de la propia llamada a listen(),
*          antes de que esta retorne. Cualquier connect() a esa señal debe
*          hacerse antes de llamar a listen(), nunca después, si se conecta
*          después, la señal ya se habrá emitido "al vacío" y nunca llegará
*          a su destinatario.
*
* @warning El callback de mensajes entrantes (onMessageCallback) se ejecuta en
*          el hilo INTERNO de libmosquitto (el que arranca mosquitto_loop_start),
*          no en el hilo principal de Qt. Por eso se usa 
*          QMetaObject::invokeMethod(..., Qt::QueuedConnection) para pasar el
*          mensaje de forma segura al slot handleMessage(), que sí se ejecuta
*          en el hilo principal.
*/
class MQTTReceiver : public IReceiver {
    Q_OBJECT

public:
    /**
    * @brief Construye el receptor MQTT.
    * 
    * @details Llama a mosquitto_lib_init(), necesario una vez por proceso
    *          antes de usar cualquier función de la librería.
    * 
    * @param[in] parent Objeto padre de Qt (gestión de memoria/jerarquía).
    * @param[in] topic Topic MQTT al que suscribirse en listen() (por defecto,
    *        "uav/telemetry").
    */
    explicit MQTTReceiver(QObject* parent = nullptr, std::string topic = "uav/telemetry");

    /**
    * @brief Destructor. Llama a stop() y a mosquitto_lib_cleanup().
    */
    ~MQTTReceiver() override;

    /**
    * @brief Conecta al broker MQTT y se suscribe al topic configurado.
    * 
    * @details Secuencia completa: crea la instancia de mosquitto, registra el
    *          callback de mensajes, conecta al broker, se suscribe al topic con 
    *          QoS 1, y arranca el hilo interno de red de libmosquitto. Si cualquier 
    *          paso falla, se deshace lo ya hecho (mosquitto_destroy) y se devuelve false.
    * 
    * @param[in] host Dirección del broker MQTT. A diferencia de UDP/TCP, aquí sí
    *        se usa de verdad: MQTTReceiver es un cliente que se conecta
    *        activamente a esta dirección.
    * @param[in] port Puerto del broker (normalmente 1883).
    * 
    * @retval true si la conexión y suscripción se completaron con éxito.
    * @retval false si falló la creación de la instancia, la conexión, la
    *         suscripción o el arranque del hilo de red.
    */
    bool listen(const std::string& host, int port) override;

    /**
    * @brief Comprueba si la conexión al broker está activa.
    * 
    * @return El valor del flag interno connected.
    */
    bool isListening() const override;

    /**
    * @brief Detiene el hilo de red, se desconecta del broker y libera la instancia.
    */
    void stop() override;

    /**
    * @brief Devuelve el último mensaje recibido en el topic suscrito.
    * 
    * @return Bytes del último mensaje entregado por handleMessage(). Si
    *         llegan varios mensajes entre dos llamadas a receive(), solo se
    *         conserva el más reciente (se sobreescribe en cada mensaje).
    */
    std::vector<uint8_t> receive() override;

private slots:
    /**
    * @brief Slot que procesa el mensaje ya en el hilo principal de Qt.
    * 
    * @details Se invoca de forma segura desde onMessageCallback() vía
    *          QMetaObject::invokeMethod. Guarda el payload y emite
    *          dataReceived().
    * 
    * @param[in] payload Bytes del mensaje MQTT recibido.
    */
    void handleMessage(const QByteArray& payload);

private:
    struct mosquitto* mosq = nullptr;  ///< Instancia de cliente libmosquitto, creada en listen().
    bool connected = false;            ///< Estado de conexión.
    std::string topic;                 ///< Topic MQTT al que se suscribe este receptor.
    QByteArray lastMessage;            ///< Último mensaje recibido, devuelto por receive().

    /**
    * @brief Callback de libmosquitto invocado por cada mensaje entrante.
    * 
    * @details Función estática porque la API en C de libmosquitto requiere
    *          un puntero a función plano, no un método de instancia. Recupera
    *          la instancia real a través del puntero de datos de usuario
    *          (obj, fijado como this en mosquitto_new) y reenvía el mensaje
    *          de forma segura al hilo de Qt mediante QMetaObject::invokeMethod.
    * 
    * @param[in] mosq Instancia de mosquitto que disparó el callback.
    * @param[in] obj Puntero de datos de usuario: la instancia real de MQTTReceiver.
    * @param[in] msg Mensaje MQTT entrante (topic, payload, longitud, QoS...).
    */
    static void onMessageCallback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg);
};