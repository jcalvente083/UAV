/**
* @file IVideoReceiver.hpp
*
* @brief Puerto para receptores de vídeo.
*
* @details Esta clase define la interfaz que debe implementar el receptor del canal
*          de vídeo del dron. A diferencia de IReceiver (telemetría, conmutable entre
*          UDP/TCP/MQTT), el vídeo viaja siempre por UDP en un puerto fijo, por eso este 
*          puerto es más simple: no necesita host (siempre escucha localmente) ni distinción 
*          entre transportes.
*
* @author Jesús David Calvente Zapata
*
*/
#pragma once
#include <QObject>
#include <QByteArray>

/**
* @class IVideoReceiver
*
* @brief Puerto (arquitectura hexagonal) para la recepción del canal de vídeo del dron.
*
* @details Define el contrato que debe cumplir cualquier adaptador que reciba el
*          flujo de vídeo (UDPVideoReceiver). El vídeo se recibe como
*          frames JPEG completos, sin decodificar.
*
* @note A diferencia de IReceiver, listen() no recibe un host: el vídeo siempre
*       se escucha en el puerto fijo definido por el contrato (5001), sin
*       importar qué protocolo de telemetría se haya elegido en la conexión.
*/
class IVideoReceiver : public QObject {
    Q_OBJECT
public:
    /**
    * @brief Constructor de la clase IVideoReceiver.
    *
    * @param[in] parent Puntero al objeto padre de tipo QObject.
    */
    explicit IVideoReceiver(QObject* parent = nullptr) : QObject(parent) {}

    /**
    * @brief Destructor virtual de la clase IVideoReceiver.
    *
    * @details Asegura que los destructores de las clases derivadas se llamen
    *          correctamente al eliminar un objeto a través de un puntero a la
    *          clase base IVideoReceiver.
    */
    virtual ~IVideoReceiver() = default;

    /**
    * @brief Inicia la escucha del canal de vídeo en el puerto especificado.
    *
    * @param[in] port Puerto UDP en el que escuchar los frames entrantes
    *        (por defecto, 5001).
    *
    * @retval true si se inició la escucha correctamente.
    * @retval false si ocurrió un error.
    */
    virtual bool listen(int port) = 0;

    /**
    * @brief Detiene la escucha del canal de vídeo y libera el socket asociado.
    */
    virtual void stop() = 0;

signals:
    /**
    * @brief Señal emitida cada vez que llega un frame de vídeo completo.
    *
    * @param[in] frameData Bytes crudos de un frame JPEG sin decodificar. El
    *        receptor de la señal es responsable de decodificarlo antes de mostrarlo.
    */
    void frameReceived(const QByteArray& frameData);
};