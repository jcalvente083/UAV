/**
* @file IReceiver.hpp
*
* @brief Puerto para receptores de datos.
*
* @details Esta clase define la interfaz que deben implementar los receptores de datos. Proporciona métodos para iniciar y detener la escucha, así como para recibir datos.
*
* @author Jesús David Calvente Zapata
*
*/
#pragma once
#include <QObject>
#include <vector>
#include <cstdint>
#include <string>

/**
* @class IReceiver
*
* @brief Puerto para receptores de telemetría en la estación de tierra.
*
* @details Esta clase define la interfaz que deben implementar los receptores de datos
*          (UDPReceiver, TCPReceiver, MQTTReceiver). Proporciona métodos para iniciar y
*          detener la escucha, así como para recibir datos.
*
* @note El comportamiento de listen() y de las señales varía según la implementación:
*      
*       - UDPReceiver/TCPReceiver: actúan como servidor pasivo, escuchando en
*         cualquier interfaz de red; el parámetro host se ignora.
*      
*       - MQTTReceiver: actúa como cliente activo, conectándose de verdad a la
*         dirección host especificada (el broker MQTT).
*
*       Cualquier código que use IReceiver debe asumir que listen() puede
*       completarse de forma síncrona (bloqueante, como en MQTT) o disparar
*       deviceConnected() de forma asíncrona más adelante (como en UDP/TCP).
*       Cualquier connect() a sus señales debe hacerse antes de llamar a
*       listen(), nunca después, para no perder una emisión síncrona.
*/
class IReceiver : public QObject {
    Q_OBJECT

public:
    /**
    * @brief Constructor de la clase IReceiver.
    * 
    * @details Este constructor inicializa un objeto de la clase IReceiver. Se puede especificar un objeto padre para la gestión de memoria y la jerarquía de objetos en Qt.
    *
    * @param[in] parent Puntero al objeto padre de tipo QObject.
    *
    */
    explicit IReceiver(QObject* parent = nullptr) : QObject(parent) {}

    /**
    * @brief Destructor virtual de la clase IReceiver.
    *
    * @details Este destructor virtual asegura que los destructores de las clases derivadas se llamen correctamente al eliminar un objeto a través de un puntero a la clase base IReceiver.
    *
    */
    virtual ~IReceiver() = default;

    /**
    * @brief Inicia la escucha/conexión según el transporte concreto.
    *
    * @param[in] host Dirección del origen de los datos. Solo se usa de verdad
    *        en implementaciones de tipo cliente, en implementaciones de tipo 
    *        servidor pasivo (UDPReceiver, TCPReceiver) se ignora.
    * @param[in] port Puerto de escucha o conexión, según el transporte.
    *
    * @retval true si la escucha/conexión se inició correctamente.
    * @retval false si ocurrió un error.
    *
    * @warning Puede emitir deviceConnected() de forma síncrona, antes de
    *          retornar. Conectar la señal deviceConnected antes de llamar 
    *          a este método, nunca después, para no perder una emisión síncrona.
    */
    virtual bool listen(const std::string& host, int port) = 0;

    /**
    * @brief Verifica si el receptor está escuchando o conectado.
    *
    * @return true si el receptor está escuchando/conectado.
    * @retval false si el receptor no está escuchando/conectado.
    *
    */
    virtual bool isListening() const = 0;

    /**
    * @brief Detiene la escucha/conexión y libera los recursos de red asociados.
    *
    */
    virtual void stop() = 0;

    /**
    * @brief Devuelve el último mensaje recibido.
    *
    * @return Vector de bytes con el último mensaje completo recibido desde
    *         la última llamada a receive(). Si no ha llegado ningún mensaje
    *         nuevo desde la última consulta, el comportamiento depende de la
    *         implementación concreta.
    */
    virtual std::vector<uint8_t> receive() = 0;

signals:

    /**
    * @brief Señal emitida cuando se establece la conexión con el origen de datos.
    *
    * @details Su significado exacto depende de la implementación:
    *          - UDPReceiver: llegada del primer datagrama.
    *          - TCPReceiver: aceptación de una conexión entrante.
    *          - MQTTReceiver: conexión y suscripción exitosa al broker.
    */
    void deviceConnected();

    /**
    * @brief Señal emitida cuando se reciben datos nuevos, listos para consultarse con receive().
    */
    void dataReceived();
};