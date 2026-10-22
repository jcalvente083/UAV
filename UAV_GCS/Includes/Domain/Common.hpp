/**
* @file Common.hpp
* 
* @brief Este archivo contiene definiciones de estructuras de datos comunes utilizadas en el proyecto.
* 
* @details Este archivo define las estructuras de datos ImageData, Detection y TelemetryData, que se utilizan 
* para representar información usada en el proyecto.
* 
* @author Jesús David Calvente Zapata.
*
*/
#pragma once
#include <vector>
#include <string>
#include <cstdint>


/**
* @brief Estructura que representa los datos de una imagen.
* 
* @details Esta estructura contiene información sobre el ancho, alto, número de canales y los datos de píxeles de una imagen.
*
* @note Los datos de la imagen se almacenan en un vector de bytes (uint8_t).
*/
struct ImageData {
    int width;                          ///< Ancho de la imagen en píxeles.
    int height;                         ///< Alto de la imagen en píxeles.
    int channels;                       ///< Número de canales de la imagen.
    std::vector<uint8_t> data;          ///< Datos de la imagen.
};


/**
* @brief Estructura que representa los datos de una detección.
*
* @details Esta estructura contiene información sobre la posición, tamaño y confianza de una detección.
*
* @note Las coordenadas (x, y, width, height) están NORMALIZADAS entre 0 y 1,
*       como fracción del ancho/alto del frame original, no son píxeles absolutos.
*       Esto permite que las cajas se muestren correctamente sin importar la
*       resolución real del vídeo recibido en la estación de tierra.
*/
struct Detection {
    double x;                           ///< Coordenada x de la esquina superior izquierda, normalizada (0-1) respecto al ancho del frame.
    double y;                           ///< Coordenada y de la esquina superior izquierda, normalizada (0-1) respecto al alto del frame.
    double width;                       ///< Ancho del cuadro delimitador, normalizado (0-1) respecto al ancho del frame.
    double height;                      ///< Alto del cuadro delimitador, normalizado (0-1) respecto al alto del frame.
    float confidence;                   ///< Nivel de confianza de la detección, en el rango [0, 1].
    int class_id;                       ///< Índice de clase COCO (0-79) de la detección.
};

/**
* @brief Estructura que representa los datos de telemetría.
*
* @details Esta estructura contiene información sobre el timestamp, batería, velocidad, altitud,
*          posición y detecciones del vehículo, lista para serializarse a JSON y enviarse a la
*          estación de tierra.
*/
struct TelemetryData {
    int64_t timestamp = 0;              ///< Timestamp de la telemetría en milisegundos (epoch, UTC).
    double battery = 0.0;               ///< Nivel de batería del vehículo, en porcentaje (0-100).
    double velocity = 0.0;              ///< Velocidad del vehículo, en m/s.
    double altitude = 0.0;              ///< Altitud del vehículo, en metros.
    std::string coordinates = "";       ///< Coordenadas GPS en formato "lat,lon" (ej. "40.4168,-3.7038").
    std::vector<Detection> detections;  ///< Detecciones realizadas sobre el frame de vídeo correspondiente.
};