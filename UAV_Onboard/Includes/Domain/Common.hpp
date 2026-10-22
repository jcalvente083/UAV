/**
* @file Common.hpp
*
* @brief Estructuras de datos de dominio compartidas por todo el sistema embarcado.
*
* @details Define ImageData, Detection y TelemetryData. Es un módulo de dominio
*          puro: solo usa la STL, sin dependencias de OpenCV, ONNX Runtime ni Qt,
*          de forma que es idéntico al Common.hpp de la estación de tierra
*          (UAV_GCS) y ambos proyectos comparten exactamente el mismo modelo
*          de datos.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include <vector>
#include <string>
#include <cstdint>

/**
* @brief Frame de imagen en memoria, independiente de cualquier librería de visión.
*
* @details Contiene las dimensiones, el número de canales y los píxeles en bruto
*          de un frame. Es el formato que intercambian los puertos IVideoSource,
*          IDetector e IVideoSink, para que ninguno de ellos dependa de cv::Mat.
*
* @note Los píxeles se almacenan fila a fila (row-major) y de forma contigua,
*       con channels bytes por píxel. Cuando el frame procede de OpenCvVideoSource,
*       el orden de canales es BGR (el de OpenCV), no RGB.
*/
struct ImageData {
    int width;                          ///< Ancho de la imagen en píxeles.
    int height;                         ///< Alto de la imagen en píxeles.
    int channels;                       ///< Número de canales por píxel (3 para BGR).
    std::vector<uint8_t> data;          ///< Píxeles en bruto, contiguos, tamaño width * height * channels.
};

/**
* @brief Resultado de una detección de objeto.
*
* @details Caja delimitadora, confianza y clase de un objeto detectado por el
*          modelo. Es lo que devuelve IDetector::detect() y lo que viaja dentro
*          del JSON de telemetría hacia la estación de tierra.
*
* @note Las coordenadas (x, y, width, height) están NORMALIZADAS entre 0 y 1,
*       como fracción del ancho/alto del frame original, no en píxeles. La
*       normalización se hace en el propio dron (OnnxYoloAdapter), tras deshacer
*       el letterbox, para que la estación de tierra pueda dibujar las cajas
*       correctamente sea cual sea la resolución del vídeo que reciba, y sin
*       depender de la resolución de entrada del modelo (480 o 640).
*/
struct Detection {
    double x;                           ///< Coordenada x de la esquina superior izquierda, normalizada (0-1) respecto al ancho del frame.
    double y;                           ///< Coordenada y de la esquina superior izquierda, normalizada (0-1) respecto al alto del frame.
    double width;                       ///< Ancho de la caja, normalizado (0-1) respecto al ancho del frame.
    double height;                      ///< Alto de la caja, normalizado (0-1) respecto al alto del frame.
    float confidence;                   ///< Confianza de la detección, en el rango [0, 1].
    int class_id;                       ///< Índice de clase COCO (0-79).
};

/**
* @brief Muestra de telemetría del dron, lista para serializarse y enviarse.
*
* @details Agrupa el estado del vehículo (batería, velocidad, altitud, posición)
*          y las detecciones más recientes. ITelemetryProvider rellena todos los
*          campos salvo detections, que los añade EdgeStreamingService justo antes
*          de serializar el paquete a JSON (ver TelemetrySerializer).
*/
struct TelemetryData {
    int64_t timestamp = 0;              ///< Momento del envío, en milisegundos desde epoch (UTC). La estación de tierra lo usa para estimar la latencia.
    double battery = 0.0;               ///< Nivel de batería, en porcentaje (0-100).
    double velocity = 0.0;              ///< Velocidad, en m/s.
    double altitude = 0.0;              ///< Altitud, en metros.
    std::string coordinates = "";       ///< Posición GPS en formato "lat,lon" (ej. "40.416800,-3.703800").
    std::vector<Detection> detections;  ///< Últimas detecciones disponibles; pueden repetirse entre paquetes si no ha terminado una inferencia nueva.
};