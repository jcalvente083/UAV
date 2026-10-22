/**
* @file UDPVideoReceiver.hpp
*
* @brief Adaptador UDP para el puerto IVideoReceiver.
*
* @details Implementa la recepción del canal de vídeo del dron por UDP, en el
*          puerto fijo definido a priori (5001). Este envío se hace de forma
*          independiente al protocolo de telemetría elegido.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include "Ports/IVideoReceiver.hpp" 
#include <QUdpSocket>

/**
* @class UDPVideoReceiver
*
* @brief Adaptador (arquitectura hexagonal) que implementa IVideoReceiver sobre UDP.
*
* @details Escucha datagramas UDP entrantes mediante un QUdpSocket vinculado a
*          QHostAddress::Any. Cada datagrama recibido se asume como un frame
*          JPEG completo, y se reenvía sin decodificar a quien esté conectado 
*          a frameReceived().
*
* @note A diferencia de UDPReceiver (telemetría), que descarta datagramas
*       intermedios y solo conserva el más reciente, UDPVideoReceiver emite
*       frameReceived() para todos los datagrama pendiente en la cola sin
*       descarta aquí. Esto es intencional: para telemetría solo
*       importar ningún frame.
*/
class UDPVideoReceiver : public IVideoReceiver {
    Q_OBJECT
public:
    /**
    * @brief Construye el receptor de vídeo y conecta la señal interna de Qt.
    * @param[in] parent Objeto padre de Qt.
    */
    UDPVideoReceiver(QObject* parent = nullptr);

    /**
    * @brief Destructor. Llama a stop() para cerrar el socket de forma ordenada.
    */
    ~UDPVideoReceiver() override;

    /**
    * @brief Vincula el socket UDP al puerto de vídeo, en cualquier interfaz.
    * @param[in] port Puerto local en el que escuchar (definido en 5001).
    * @retval true si el bind() tuvo éxito.
    * @retval false si el puerto ya está en uso o el bind falla por otro motivo.
    */
    bool listen(int port) override;

    /**
    * @brief Cierra el socket UDP si estaba abierto.
    */
    void stop() override;

private slots:
    /**
    * @brief Slot interno conectado a QUdpSocket::readyRead.
    * @details Recorre los datagramas pendientes (no solo el último) y
    *          emite frameReceived() una vez por cada uno, en orden de llegada.
    */
    void onReadyRead();

private:
    QUdpSocket* socket;     ///< Socket UDP subyacente, vinculado en listen().
};