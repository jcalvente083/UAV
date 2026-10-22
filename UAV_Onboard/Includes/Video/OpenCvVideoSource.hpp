/**
* @file OpenCvVideoSource.hpp
*
* @brief Adaptador de IVideoSource basado en cv::VideoCapture de OpenCV.
*
* @details Permite leer frames de una cámara local, de un fichero de vídeo o de
*          un stream de red directo, entregándolos como ImageData para que el
*          resto del sistema no dependa de OpenCV.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include <opencv2/opencv.hpp>

#include "Ports/IVideoSource.hpp"

/**
* @class OpenCvVideoSource
*
* @brief Adaptador (arquitectura hexagonal) que implementa IVideoSource con cv::VideoCapture.
*
* @details La decodificación la hace el backend de OpenCV que corresponda
*          (FFmpeg, Media Foundation en Windows...). En el contenedor Docker
*          (Linux) los ficheros .mp4 se pueden leer porque la imagen instala
*          OpenCV con soporte FFmpeg.
*
* @note Se hace UNA copia por frame (de cv::Mat a ImageData). Es el precio de que
*       el dominio no dependa de OpenCV: los puertos solo conocen ImageData.
*       Para minimizar su coste, tanto el cv::Mat interno como el vector de
*       ImageData se reutilizan entre llamadas, sin reservar memoria nueva
*       mientras la resolución no cambie.
*/
class OpenCvVideoSource : public IVideoSource {
public:
    /**
    * @brief Construye la fuente de vídeo.
    *
    * @param[in] loop Si es true, al llegar al final de un fichero se rebobina al
    *            primer frame y se sigue leyendo indefinidamente. Útil para
    *            pruebas y demos con un vídeo corto. No tiene efecto con cámaras
    *            ni streams en directo, que no tienen "final" que rebobinar.
    */
    explicit OpenCvVideoSource(bool loop = false);

    /**
    * @brief Abre la fuente indicada.
    *
    * @details Si source contiene solo dígitos (ej. "0"), se interpreta como
    *          índice de cámara local; en cualquier otro caso se pasa tal cual
    *          a OpenCV como ruta de fichero o URL de stream (RTSP/HTTP).
    *          Las URL de páginas como YouTube no son streams directos y OpenCV
    *          no puede abrirlas.
    *
    * @param[in] source Índice de cámara, ruta de fichero o URL de stream.
    *
    * @retval true si la fuente se abrió correctamente.
    * @retval false si no existe, no se pudo abrir o el códec no está soportado.
    */
    bool open(const std::string& source) override;

    /**
    * @brief Indica si la fuente está abierta.
    * @return true si cv::VideoCapture está abierto.
    */
    bool isOpened() const override;

    /**
    * @brief Lee el siguiente frame y lo copia a ImageData.
    *
    * @details Si la lectura falla y el modo bucle está activo, rebobina al
    *          frame 0 y reintenta una vez. Si el cv::Mat leído no es contiguo
    *          en memoria, se clona para que lo sea antes de copiarlo, ya que
    *          ImageData asume píxeles contiguos fila a fila.
    *
    * @param[out] frame Destino del frame, en BGR (orden de canales de OpenCV).
    *
    * @retval true si se obtuvo un frame válido.
    * @retval false si no hay más frames (fin de fichero sin bucle) o la lectura falló.
    */
    bool read(ImageData& frame) override;

    /**
    * @brief Libera la cámara, el fichero o el stream.
    */
    void close() override;

private:
    cv::VideoCapture capture;   ///< Objeto de captura de OpenCV que abstrae cámara, fichero o stream.
    cv::Mat frameBuffer;        ///< Buffer intermedio reutilizado en cada read() para no reservar memoria por frame.
    bool loopEnabled;           ///< Si es true, se rebobina al llegar al final de un fichero.
};