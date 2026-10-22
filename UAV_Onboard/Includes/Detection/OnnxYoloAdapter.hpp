/**
* @file OnnxYoloAdapter.hpp
*
* @brief Adaptador de IDetector basado en YOLOv8n y ONNX Runtime.
*
* @details Implementa la detección de objetos a bordo: preprocesado con OpenCV,
*          inferencia con ONNX Runtime en CPU y postprocesado (decodificación de
*          la salida de YOLOv8 y NMS) para devolver detecciones de dominio.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include <opencv2/core.hpp>

#include "Ports/IDetector.hpp"
#include "Domain/Common.hpp"

/**
* @class OnnxYoloAdapter
*
* @brief Adaptador (arquitectura hexagonal) que implementa IDetector con YOLOv8n sobre ONNX Runtime.
*
* @details Pipeline de cada llamada a detect():
*          1. preprocess(): letterbox al tamaño de entrada del modelo y
*             conversión a tensor NCHW float32 normalizado.
*          2. Inferencia con Ort::Session::Run().
*          3. processOutput(): decodificación de la salida, umbral de
*             confianza, NMS por clase y normalización de coordenadas.
*
* @warning Las constantes de tamaño están fijadas para el modelo de 480 px
*          (entrada [1, 3, 480, 480], salida [1, 84, 4725]). El modelo indicado
*          en config.json DEBE coincidir con ellas: si se cargara un modelo de
*          640 px (salida [1, 84, 8400]) sin actualizar estas constantes, la
*          decodificación leería posiciones incorrectas del tensor y las
*          detecciones serían basura, sin ningún error visible. Hacerlas
*          configurables (calculando el número de anclas a partir del tamaño)
*          queda como mejora futura.
*
* @note Se usa el modelo FP32. Se probó una versión cuantizada a INT8 y, medida
*       en las mismas condiciones (latencia y memoria), no aportó una mejora
*       neta que justificara su uso. La reducción de latencia real se obtuvo
*       bajando la resolución de entrada de 640 a 480 px (aprox. 40-50 % menos
*       de tiempo de inferencia).
*/
class OnnxYoloAdapter : public IDetector {
public:
    /**
    * @brief Construye el adaptador y configura las opciones de sesión de ONNX Runtime.
    *
    * @details Usa un único hilo de inferencia (SetIntraOpNumThreads(1)): el
    *          sistema está pensado para ejecutarse con medio núcleo de CPU
    *          (Docker con --cpus=0.5), donde más hilos no aportan paralelismo
    *          real y solo añaden contención. El nivel de optimización del grafo
    *          es ORT_ENABLE_EXTENDED.
    */
    OnnxYoloAdapter();

    /**
    * @brief Destructor. Los recursos de ONNX Runtime se liberan automáticamente.
    */
    ~OnnxYoloAdapter() override;

    /**
    * @brief Carga el modelo ONNX y cachea los nombres de entrada y salida.
    *
    * @details Los nombres de los tensores (en YOLOv8, "images" y "output0") se
    *          consultan una única vez aquí y se guardan, en lugar de pedirlos al
    *          modelo en cada frame, para no repetir ese coste en el bucle
    *          principal. La ruta se convierte a std::filesystem::path porque en
    *          Windows ONNX Runtime espera rutas de caracteres anchos.
    *
    * @param[in] modelPath Ruta al fichero .onnx (campo modelo de config.json).
    *
    * @retval true si la sesión se creó correctamente.
    * @retval false si ONNX Runtime lanzó una excepción (fichero inexistente,
    *         modelo inválido, DLL incompatible...). El error se muestra por consola.
    */
    bool loadModel(const std::string& modelPath) override;

    /**
    * @brief Indica si hay un modelo cargado.
    * @return true tras un loadModel() exitoso.
    */
    bool isLoaded() const override;

    /**
    * @brief Ejecuta el pipeline completo de detección sobre un frame.
    *
    * @details El tensor de entrada se construye directamente sobre la memoria
    *          del blob generado por preprocess(), sin copia adicional. Los
    *          tiempos de preprocesado e inferencia se miden con steady_clock
    *          (reloj monótono) y se imprimen por consola como [DEBUG]; esa
    *          traza es de diagnóstico y conviene retirarla o hacerla opcional
    *          en la versión final.
    *
    * @param[in] image Frame en BGR.
    *
    * @return Detecciones con coordenadas normalizadas (0-1). Vector vacío si no
    *         hay modelo cargado o no se detecta nada por encima del umbral.
    */
    std::vector<Detection> detect(const ImageData& image) override;

private:
    Ort::Env env;                           ///< Entorno de ONNX Runtime (logging a nivel WARNING).
    Ort::SessionOptions session_options;    ///< Opciones de sesión: un hilo, optimización EXTENDED.
    std::unique_ptr<Ort::Session> session;  ///< Sesión de inferencia; nullptr hasta que loadModel() tiene éxito.

    std::string inputName;                  ///< Nombre del tensor de entrada, cacheado en loadModel().
    std::string outputName;                 ///< Nombre del tensor de salida, cacheado en loadModel().

    /**
    * @brief Parámetros del letterbox aplicado en preprocess(), necesarios para deshacerlo.
    */
    struct LetterboxInfo {
        float scale;    ///< Factor de escala aplicado al frame original para encajarlo en el tamaño de entrada.
        int padX;       ///< Relleno horizontal añadido a cada lado, en píxeles del espacio del modelo.
        int padY;       ///< Relleno vertical añadido arriba y abajo, en píxeles del espacio del modelo.
    };

    bool model_loaded;                      ///< true tras un loadModel() exitoso.

    /**
    * @brief Decodifica la salida cruda del modelo en detecciones de dominio.
    *
    * @details Pasos:
    *          1. La salida [1, 84, N] se lee directamente en orden channel-major
    *             (raw_data[canal * N + ancla]), sin transponerla a otra matriz,
    *             para no hacer una copia completa del tensor.
    *          2. Por cada ancla: clase = la de mayor puntuación entre las 80,
    *             confianza = esa puntuación; se descartan las inferiores a 0.25.
    *          3. Conversión de centro/ancho/alto a esquina superior izquierda y
    *             deshacer el letterbox: primero se RESTA el relleno (padX/padY) y
    *             después se divide por la escala. El orden importa: el letterbox
    *             es centrado, así que sin restar el relleno las cajas quedarían
    *             desplazadas.
    *          4. NMS por clase (IoU 0.45), hecho a mano agrupando por class_id y
    *             llamando a cv::dnn::NMSBoxes por cada grupo. No se usa
    *             cv::dnn::NMSBoxesBatched porque no existe en todas las versiones
    *             de OpenCV (por ejemplo, la 4.5.4 de Ubuntu 22.04 que se usa en
    *             Docker), y así el mismo código compila en Windows y en Linux.
    *          5. Recorte de cada caja a los límites del frame y normalización a 0-1.
    *
    * @param[in] raw_data Puntero a la salida del modelo ([1, 84, N], float32).
    * @param[in] letterboxInfo Escala y relleno usados en preprocess().
    * @param[in] origWidth Ancho del frame original, en píxeles.
    * @param[in] origHeight Alto del frame original, en píxeles.
    *
    * @return Detecciones finales tras NMS, normalizadas respecto al frame original.
    */
    std::vector<Detection> processOutput(float* raw_data, const LetterboxInfo& letterboxInfo, int origWidth, int origHeight);

    /**
    * @brief Prepara un frame para la entrada del modelo.
    *
    * @details El ImageData se envuelve como cv::Mat sin copiar sus datos. Se
    *          reescala manteniendo la proporción y se centra sobre un lienzo
    *          cuadrado relleno de gris (114, 114, 114), el mismo letterbox que
    *          usa Ultralytics al entrenar el modelo. Finalmente
    *          cv::dnn::blobFromImage normaliza a [0, 1], convierte BGR a RGB y
    *          reordena a NCHW en una sola llamada.
    *
    * @param[in] image Frame original en BGR.
    * @param[out] outInfo Escala y relleno aplicados, que processOutput() necesita
    *             para devolver las cajas al espacio del frame original.
    *
    * @return Blob float32 con forma [1, 3, tamaño, tamaño], listo para el tensor de entrada.
    */
    cv::Mat preprocess(const ImageData& image, LetterboxInfo& outInfo);
};