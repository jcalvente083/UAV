/**
* @file TelemetrySerializer.hpp
*
* @brief Serialización de TelemetryData al JSON del contrato de protocolo.
*
* @details Convierte una muestra de telemetría en el mensaje JSON que viaja por
*          cualquiera de los transportes (UDP, TCP o MQTT) y que la estación de
*          tierra interpreta en DashboardWindow::parseTelemetry(). Ambos lados
*          deben usar exactamente las mismas claves.
*
*          Se construye a mano con std::ostringstream en lugar de con una
*          librería JSON: el esquema es pequeño y fijo, y así el binario no
*          depende de nada más.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include <sstream>
#include <iomanip>
#include "Domain/Common.hpp"

/**
* @brief Genera el mensaje JSON de telemetría.
*
* @details Ejemplo de salida (en una sola línea, sin espacios; aquí se muestra
*          con formato para que sea legible):
* @code{.json}
* {
*   "timestamp": 1788795430709,
*   "bateria": 99.99,
*   "velocidad": 13.00,
*   "altura": 50.25,
*   "coordenadas": "40.416805,-3.703700",
*   "detecciones": [
*     { "class_id": 2, "confidence": 0.3819,
*       "x": 0.533088, "y": 0.675817, "width": 0.045588, "height": 0.097386 }
*   ]
* }
* @endcode
*
*          Las claves del estado de vuelo están en español (bateria, velocidad,
*          altura, coordenadas, detecciones) por compatibilidad con el parser de
*          la estación de tierra; las de cada detección (class_id, confidence,
*          x, y, width, height) están en inglés, según el contrato.
*
*          Precisión numérica:
*          - batería, velocidad y altura: 2 decimales.
*          - confidence: 4 decimales.
*          - x, y, width, height: 6 decimales, suficiente para coordenadas
*            normalizadas (0-1) sin pérdida apreciable al desnormalizar.
*          - timestamp: entero, en milisegundos desde epoch.
*
* @param[in] data Muestra de telemetría, con las detecciones ya añadidas.
*
* @return Cadena JSON compacta, lista para entregarse a ITelemetrySink::publish().
*
* @note No escapa caracteres especiales en coordinates: es seguro porque ese
*       texto lo genera el propio dron con el formato "lat,lon". Tampoco
*       contempla valores NaN o infinitos, que producirían un JSON no válido.
*       El separador decimal es el punto mientras no se cambie la configuración
*       regional (locale) global del programa.
*/
inline std::string telemetryToJson(const TelemetryData& data) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"timestamp\":" << data.timestamp << ",";
    oss << std::fixed << std::setprecision(2);
    oss << "\"bateria\":" << data.battery << ",";
    oss << "\"velocidad\":" << data.velocity << ",";
    oss << "\"altura\":" << data.altitude << ",";
    oss << "\"coordenadas\":\"" << data.coordinates << "\",";
    oss << "\"detecciones\":[";
    for (size_t i = 0; i < data.detections.size(); ++i) {
        const Detection& d = data.detections[i];
        oss << "{"
            << "\"class_id\":" << d.class_id << ","
            << std::setprecision(4)
            << "\"confidence\":" << d.confidence << ","
            << std::setprecision(6)
            << "\"x\":" << d.x << ","
            << "\"y\":" << d.y << ","
            << "\"width\":" << d.width << ","
            << "\"height\":" << d.height
            << "}";
        if (i + 1 < data.detections.size()) oss << ",";
    }
    oss << "]";
    oss << "}";
    return oss.str();
}