/**
* @file TCPReceiver.hpp
*
* @brief Adaptador TCP para el puerto IReceiver.
*
* @details Implementa la recepción de telemetría por TCP como servidor: escucha
*          conexiones entrantes y acepta un único cliente activo a la vez.
*          Los mensajes se delimitan mediante framing de longitud-prefijo
*          de 4 bytes en big-endian ([uint32][payload]), simétrico al framing
*          que usa TCPTelemetrySink en el lado del dron.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once

#include <QTcpServer>
#include <QTcpSocket>

#include "Ports/IReceiver.hpp"

/**
* @class TCPReceiver
*
* @brief Adaptador (arquitectura hexagonal) que implementa IReceiver sobre TCP.
*
* @details Usa un QTcpServer para escuchar conexiones entrantes y un único
*          QTcpSocket para el cliente activo. Si llega una segunda conexión 
*          mientras ya hay un cliente activo, se rechaza de inmediato.
*
* @note Como TCP es orientado a conexión, deviceConnected() se emite en cuanto
*       se acepta la conexión entrante, no al primer mensaje (a diferencia de
*       UDPReceiver, que necesita esperar el primer datagrama porque UDP no
*       tiene un concepto real de "conexión establecida").
*/
class TCPReceiver : public IReceiver {
    Q_OBJECT

public:
    /**
    * @brief Construye el receptor TCP y conecta la señal de nuevas conexiones.
    * 
    * @param[in] parent Objeto padre de Qt.
    */
    TCPReceiver(QObject* parent = nullptr);

    /**
    * @brief Destructor. Llama a stop() para cerrar cliente y servidor.
    */
    ~TCPReceiver();

    /**
    * @brief Pone al servidor TCP a escuchar conexiones entrantes.
    * 
    * @param[in] host Ignorado en esta implementación: el servidor escucha en
    *        cualquier interfaz (QHostAddress::Any), no filtra por origen.
    * @param[in] port Puerto local en el que escuchar.
    * 
    * @retval true si el servidor se puso a escuchar correctamente.
    * @retval false si el puerto ya está en uso o falla por otro motivo.
    */
    bool listen(const std::string& host, int port) override;

    /**
    * @brief Comprueba si el servidor sigue escuchando conexiones entrantes.
    * 
    * @return true si el QTcpServer está activo; false en caso contrario.
    */
    bool isListening() const override;

    /**
    * @brief Desconecta al cliente activo (si lo hay) y cierra el servidor.
    */
    void stop() override;

    /**
    * @brief Devuelve el último mensaje completo reensamblado.
    * 
    * @return Bytes del último mensaje completo recibido según el framing de
    *         longitud-prefijo. Si llegaron varios mensajes completos en el
    *         mismo evento readyRead(), solo se conserva el más reciente,
    *         mismo criterio que UDPReceiver.
    */
    std::vector<uint8_t> receive() override;

private slots:
    /**
    * @brief Slot conectado a QTcpServer::newConnection.
    * 
    * @details Acepta la conexión entrante solo si no hay ya un cliente activo;
    *          si lo hay, la rechaza y la cierra inmediatamente. Al aceptar,
    *          emite deviceConnected() de inmediato.
    */
    void onNewConnection();

    /**
    * @brief Slot conectado a QTcpSocket::readyRead del cliente activo.
    * 
    * @details Acumula los bytes entrantes en recvBuffer y, en un bucle,
    *          extrae todos los mensajes completos posibles según el framing
    *          [uint32 big-endian][payload]: primero lee los 4 bytes de
    *          longitud, comprueba si el payload completo ya ha llegado, y si
    *          es así lo extrae y lo guarda en lastMessage (sobreescribiendo
    *          cualquier mensaje anterior no consumido). Si el mensaje aún no
    *          ha llegado entero, se detiene y espera más datos en la próxima
    *          llamada. Emite dataReceived() si se completó al menos un
    *          mensaje en esta pasada.
    */
    void onReadyRead();

    /**
    * @brief Slot conectado a QTcpSocket::disconnected del cliente activo.
    * 
    * @details Libera el socket del cliente y limpia el buffer de recepción,
    *          dejando el receptor listo para aceptar una nueva conexión
    *          entrante en el futuro.
    */
    void onClientDisconnected();

private:
    QTcpServer* server;        ///< Servidor TCP que acepta la conexión entrante del dron.
    QTcpSocket* clientSocket;  ///< Socket del cliente activo, o nullptr si no hay ninguno conectado.
    QByteArray recvBuffer;     ///< Buffer de acumulación de bytes aún sin reensamblar en un mensaje completo.
    QByteArray lastMessage;    ///< Último mensaje completo reensamblado, devuelto por receive().
};