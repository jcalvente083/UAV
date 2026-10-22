/**
* @file UDPReceiver.hpp
*
* @brief Adaptador UDP para el puerto IReceiver.
*
* @details Implementa la recepción de telemetría por UDP como un servidor pasivo:
*          escucha en un puerto local, sin conectarse activamente a ninguna
*          dirección remota.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include "Ports/IReceiver.hpp" 
#include <QUdpSocket>

/**
* @class UDPReceiver
*
* @brief Adaptador (arquitectura hexagonal) que implementa IReceiver sobre UDP.
*
* @details Escucha datagramas UDP entrantes mediante un QUdpSocket vinculado a
*          QHostAddress::Any (cualquier interfaz de red). Considera "conectado"
*          el dron en cuanto llega el primer datagrama.
*/
class UDPReceiver : public IReceiver {
    Q_OBJECT

public:
    /**
    * @brief Construye el receptor UDP y conecta la señal interna de Qt.
    * 
    * @param parent Objeto padre de Qt.
    */
    UDPReceiver(QObject* parent = nullptr);

    /**
    * @brief Destructor. No cierra el socket explícitamente.
    */
    ~UDPReceiver();

    /**
    * @brief Vincula el socket UDP al puerto especificado, en cualquier interfaz.
    * 
    * @details host se ignora deliberadamente: UDPReceiver escucha en cualquier
    *          interfaz de red (QHostAddress::Any), no filtra por origen.
    * 
    * @param[in] host Ignorado en esta implementación (UDP es un servidor pasivo,
    *        no se conecta a ninguna dirección concreta).
    * @param[in] port Puerto local en el que escuchar.
    * 
    * @retval true si el bind() tuvo éxito.
    * @retval false si el puerto ya está en uso o el bind falla por otro motivo.
    */
    bool listen(const std::string& host, int port) override;

    /**
    * @brief Comprueba si el socket sigue vinculado.
    * 
    * @return true si el socket está a la escucha; false en cualquier otro estado.
    */
    bool isListening() const override;

    /**
    * @brief Cierra el socket UDP.
    */
    void stop() override;

    /**
    * @brief Vacía todos los datagramas pendientes y devuelve solo el último.
    * 
    * @details El bucle interno obtiene los datagramas pendientes en el
    *          socket, pero como el buffer se sobreescribe en cada iteración,
    *          al terminar solo queda el último.
    * 
    * @return Bytes del datagrama más reciente. Si llegaron varios datagramas
    *         desde la última llamada, los intermedios se descartan a propósito.
	*         Así la telemetría se mantiene lo más actual posible, sin arrastrar
    *         retraso acumulado reduciendo así la latencia.
    * 
    */
    std::vector<uint8_t> receive() override;

private slots:
    /**
    * @brief Slot interno conectado a QUdpSocket::readyRead.
    * 
    * @details isFirstPacket actúa como cerrojo de un solo uso: la primera vez
    *          que se recibe un datagrama, se emite deviceConnected() y el flag
    *          pasa a false para siempre, no vuelve a emitirse aunque el
    *          socket se quede escuchando indefinidamente. dataReceived() sí
    *          se emite en cada datagrama, incluido el primero.
    */
    void onReadyRead();

private:
    QUdpSocket* socket;     ///< Socket UDP subyacente, vinculado en listen().
    bool isFirstPacket;     ///< true hasta que llega el primer datagrama; controla la emisión única de deviceConnected().
};