/**
* @file IVideoSink.hpp
*
* @brief Puerto para el envío del vídeo del dron hacia la estación de tierra.
*
* @details Define la interfaz que debe implementar cualquier transmisor de vídeo.
*          Actualmente la implementa UDPVideoSink (JPEG sobre UDP).
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include <string>
#include "Domain/Common.hpp"

/**
* @class IVideoSink
*
* @brief Puerto (arquitectura hexagonal) para la transmisión de frames de vídeo.
*
* @details Recibe frames sin comprimir (ImageData) y se encarga de todo lo
*          necesario para enviarlos: reescalado, codificación y transporte.
*          EdgeStreamingService solo entrega el frame tal cual sale de
*          IVideoSource, sin saber cómo se comprime ni por dónde viaja.
*
* @note Según el contrato de protocolo, el vídeo viaja SIEMPRE por un canal
*       propio (UDP, puerto 5001), independiente del transporte elegido para
*       la telemetría (UDP/TCP/MQTT). Por eso hay un puerto de vídeo separado
*       de ITelemetrySink, en lugar de uno genérico de envío.
*/
class IVideoSink {
public:
    /**
    * @brief Destructor virtual.
    *
    * @details Garantiza que se llame al destructor del adaptador concreto al
    *          destruirlo a través de un puntero a IVideoSink.
    */
    virtual ~IVideoSink() = default;

    /**
    * @brief Prepara el canal de envío hacia la estación de tierra.
    *
    * @param[in] host Dirección IP de la estación de tierra (campo host de
    *            config.json). Dentro de un contenedor Docker no sirve
    *            "127.0.0.1", que apunta al propio contenedor: hay que usar la
    *            IP real del equipo donde corre la estación.
    * @param[in] port Puerto de destino del vídeo (por contrato, 5001).
    *
    * @retval true si el canal quedó listo para publicar.
    * @retval false si no se pudo crear o configurar el socket.
    */
    virtual bool start(const std::string& host, int port) = 0;

    /**
    * @brief Comprime y envía un frame.
    *
    * @details La implementación es responsable de reescalar el frame (como
    *          máximo 480 px de alto) y codificarlo antes de enviarlo. En
    *          UDPVideoSink cada frame JPEG debe caber en un único datagrama;
    *          la calidad JPEG se calibra al principio y, si un frame no cabe,
    *          se descarta y se baja la calidad para los siguientes.
    *
    * @param[in] frame Frame sin comprimir en BGR. No se conserva tras retornar.
    *
    * @retval true si el frame se envió.
    * @retval false si no se envió (canal no iniciado, error de red o frame
    *         descartado por tamaño). No es un error fatal: perder un frame
    *         puntual es aceptable y el bucle simplemente continúa.
    */
    virtual bool publish(const ImageData& frame) = 0;

    /**
    * @brief Cierra el canal de envío y libera el socket.
    */
    virtual void stop() = 0;
};