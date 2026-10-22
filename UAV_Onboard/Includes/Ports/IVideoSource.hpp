/**
* @file IVideoSource.hpp
*
* @brief Puerto para fuentes de vídeo del dron.
*
* @details Define la interfaz que debe implementar cualquier origen de frames
*          (cámara, fichero de vídeo, stream de red). Actualmente la implementa
*          OpenCvVideoSource, basado en cv::VideoCapture.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include <string>

#include "Domain/Common.hpp"

/**
* @class IVideoSource
*
* @brief Puerto (arquitectura hexagonal) para la captura de frames de vídeo.
*
* @details Modelo de lectura bajo demanda (pull): quien la usa abre la fuente
*          una vez y va pidiendo frames uno a uno con read(). Es el rol opuesto
*          a IVideoReceiver de la estación de tierra, que recibe frames ya
*          generados por red (push); por eso son puertos distintos aunque ambos
*          traten con vídeo.
*
* @note Trabaja solo con ImageData (tipo de dominio), sin exponer cv::Mat, para
*       que EdgeStreamingService no dependa de OpenCV.
*/
class IVideoSource {
public:
    /**
    * @brief Destructor virtual.
    *
    * @details Garantiza que se llame al destructor del adaptador concreto al
    *          destruirlo a través de un puntero a IVideoSource.
    */
    virtual ~IVideoSource() = default;

    /**
    * @brief Abre la fuente de vídeo.
    *
    * @details La interpretación de source depende de la implementación. En
    *          OpenCvVideoSource:
    *          - Un texto solo numérico (ej. "0") se toma como índice de cámara local.
    *          - Cualquier otro texto se trata como ruta de fichero (ej.
    *            "Models/video.mp4") o URL de stream directo (RTSP/HTTP).
    *
    * @param[in] source Identificador de la fuente, normalmente leído del campo
    *            fuente_video de config.json.
    *
    * @retval true si la fuente se abrió correctamente.
    * @retval false si no existe, el códec no está soportado o no se pudo abrir.
    */
    virtual bool open(const std::string& source) = 0;

    /**
    * @brief Indica si la fuente está abierta y lista para leer.
    *
    * @retval true si read() puede llamarse.
    * @retval false si no se ha abierto o se cerró.
    */
    virtual bool isOpened() const = 0;

    /**
    * @brief Lee el siguiente frame de la fuente.
    *
    * @details El frame se devuelve por referencia (parámetro de salida) en lugar
    *          de por valor para poder REUTILIZAR el mismo buffer en cada llamada:
    *          mientras la resolución no cambie, no se reserva memoria nueva por
    *          frame, algo relevante bajo el límite de memoria del contenedor.
    *          Qué ocurre al llegar al final de un fichero depende de la
    *          implementación (OpenCvVideoSource puede rebobinar si se construyó
    *          con loop activado).
    *
    * @param[out] frame Destino del frame leído, en BGR. Se sobrescribe por completo.
    *
    * @retval true si se leyó un frame válido.
    * @retval false si no hay más frames (fin de fichero sin bucle) o si hubo un
    *         error de lectura; en ese caso el contenido de frame no es válido.
    */
    virtual bool read(ImageData& frame) = 0;

    /**
    * @brief Cierra la fuente y libera los recursos asociados (cámara, fichero, stream).
    */
    virtual void close() = 0;
};