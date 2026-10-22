/**
* @file IDetector.hpp
*
* @brief Puerto para el detector de objetos a bordo del dron.
*
* @details Define la interfaz que debe implementar cualquier motor de inferencia
*          usado por el sistema embarcado. Actualmente la implementa
*          OnnxYoloAdapter (YOLOv8n sobre ONNX Runtime).
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include <vector>
#include <string>

#include "Domain/Common.hpp"

/**
* @class IDetector
*
* @brief Puerto (arquitectura hexagonal) para la detección de objetos en un frame.
*
* @details Cualquier motor de inferencia (ONNX Runtime, TensorRT, OpenCV DNN...)
*          que implemente este puerto puede sustituir a OnnxYoloAdapter sin que
*          EdgeStreamingService ni main.cpp tengan que cambiar una sola línea:
*          ambos conocen únicamente esta interfaz, nunca el adaptador concreto.
*
* @note La interfaz trabaja solo con tipos de dominio (ImageData, Detection),
*       sin exponer cv::Mat ni tensores de ONNX Runtime, para que el resto del
*       sistema no dependa de la librería de inferencia elegida.
*/
class IDetector {
public:
    /**
    * @brief Destructor virtual.
    *
    * @details Garantiza que se llame al destructor del adaptador concreto al
    *          destruirlo a través de un puntero a IDetector (por ejemplo, el
    *          std::unique_ptr<IDetector> de main.cpp).
    */
    virtual ~IDetector() = default;

    /**
    * @brief Carga el modelo de detección desde disco.
    *
    * @param[in] modelPath Ruta al fichero del modelo (p. ej. "Models/yolov8n_480.onnx"),
    *            normalmente leída de config.json. Las rutas relativas se resuelven
    *            respecto al directorio de trabajo del proceso.
    *
    * @retval true si el modelo se cargó correctamente y detect() ya puede usarse.
    * @retval false si el fichero no existe o el modelo no pudo cargarse.
    */
    virtual bool loadModel(const std::string& modelPath) = 0;

    /**
    * @brief Indica si hay un modelo cargado y listo para inferir.
    *
    * @retval true si detect() puede llamarse de forma segura.
    * @retval false si todavía no se ha llamado a loadModel() o la carga falló.
    */
    virtual bool isLoaded() const = 0;

    /**
    * @brief Ejecuta la detección de objetos sobre un frame.
    *
    * @details Llamada SÍNCRONA y bloqueante: incluye preprocesado, inferencia y
    *          postprocesado (NMS). Es la operación más costosa del bucle
    *          principal (del orden de 60-100 ms en desarrollo con el modelo de
    *          480 px, y varias veces más bajo restricción de CPU en Docker), por
    *          eso EdgeStreamingService no la invoca en cada frame de vídeo, sino
    *          que reutiliza las últimas detecciones mientras tanto.
    *
    * @param[in] image Frame de entrada en BGR, tal como lo entrega IVideoSource.
    *            La implementación no debe guardar referencias a él tras
    *            retornar: el buffer se reutiliza en el siguiente frame.
    *
    * @return Detecciones tras NMS, con coordenadas normalizadas (0-1) respecto
    *         al frame original (ver Detection en Common.hpp). Vector vacío si no
    *         hay ningún objeto por encima del umbral de confianza o si no hay
    *         modelo cargado.
    */
    virtual std::vector<Detection> detect(const ImageData& image) = 0;
};