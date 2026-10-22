/**
* @file Config.hpp
*
* @brief Configuración en tiempo de ejecución del sistema embarcado, cargada desde config.json.
*
* @details Permite cambiar transporte, direcciones, modelo, fuente de vídeo y
*          parámetros de la simulación sin recompilar. En el dron no hay
*          interfaz gráfica, así que este fichero es la única forma de
*          parametrizarlo.
*
*          El JSON se interpreta con expresiones regulares hechas a medida para
*          las claves conocidas, en lugar de con una librería JSON completa: el
*          fichero es pequeño y de formato fijo, y así no se añade ninguna
*          dependencia al binario.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>

/**
* @brief Parámetros de configuración del dron, con sus valores por defecto.
*
* @details Cada campo se corresponde con una clave de config.json:
*
*          | Clave JSON          | Campo               | Tipo   | Por defecto              |
*          |---------------------|---------------------|--------|--------------------------|
*          | transporte          | transport           | texto  | "udp"                    |
*          | host                | groundStationHost   | texto  | "127.0.0.1"              |
*          | puerto_telemetria   | telemetryPort       | entero | 5000                     |
*          | puerto_video        | videoPort           | entero | 5001                     |
*          | modelo              | modelPath           | texto  | "Models/yolov8n.onnx"    |
*          | fuente_video        | videoSource         | texto  | "Models/sample.mp4"      |
*          | fps_video           | videoTargetFps      | entero | 10                       |
*          | latitud_inicial     | initialLatitude     | real   | 40.4168                  |
*          | longitud_inicial    | initialLongitude    | real   | -3.7038                  |
*          | mqtt_topic          | mqttTopic           | texto  | "uav/telemetry"          |
*
* @warning El valor por defecto de modelPath apunta al modelo de 640 px, mientras
*          que OnnxYoloAdapter está fijado para el de 480 px. Si config.json no
*          define "modelo", el detector cargaría un modelo incompatible con sus
*          constantes y produciría detecciones incorrectas sin error visible.
*/
struct Config {
    std::string transport = "udp";                  ///< Transporte de telemetría: "udp", "tcp" o "mqtt". Cualquier otro valor se trata como "udp".
    std::string groundStationHost = "127.0.0.1";    ///< En UDP/TCP, IPv4 de la estación de tierra; en MQTT, dirección del broker. En Docker no sirve 127.0.0.1.
    int telemetryPort = 5000;                       ///< En UDP/TCP, puerto de la estación de tierra (5000); en MQTT, puerto del broker (normalmente 1883).
    int videoPort = 5001;                           ///< Puerto de vídeo de la estación de tierra (siempre UDP).
    std::string modelPath = "Models/yolov8n.onnx";  ///< Ruta al modelo ONNX, relativa al directorio de trabajo. Debe coincidir con las constantes de OnnxYoloAdapter.
    std::string videoSource = "Models/sample.mp4";  ///< Fuente de vídeo: índice de cámara ("0"), ruta de fichero o URL de stream.
    int videoTargetFps = 10;                        ///< Fotogramas por segundo objetivo del bucle principal. Debe ser mayor que 0.
    double initialLatitude = 40.4168;               ///< Latitud de partida del dron simulado, en grados.
    double initialLongitude = -3.7038;              ///< Longitud de partida del dron simulado, en grados.
    std::string mqttTopic = "uav/telemetry";        ///< Topic MQTT en el que se publica la telemetría.
};

/**
* @brief Extrae el valor de texto de una clave del JSON.
*
* @param[in] json Contenido completo del fichero de configuración.
* @param[in] key Nombre de la clave, sin comillas.
* @param[in] defaultValue Valor devuelto si la clave no se encuentra.
*
* @return El texto entre comillas asociado a la clave, o defaultValue.
*
* @note No admite comillas escapadas (\") dentro del valor.
*/
inline std::string extractString(const std::string& json, const std::string& key, const std::string& defaultValue) {
    std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(json, match, pattern)) {
        return match[1].str();
    }
    return defaultValue;
}

/**
* @brief Extrae el valor entero de una clave del JSON.
*
* @param[in] json Contenido completo del fichero de configuración.
* @param[in] key Nombre de la clave, sin comillas.
* @param[in] defaultValue Valor devuelto si la clave no se encuentra.
*
* @return El entero asociado a la clave, o defaultValue.
*
* @note Si el valor tiene decimales, se toma solo la parte entera. Si está entre
*       comillas ("10" en lugar de 10), no se reconoce y se usa defaultValue.
*/
inline int extractInt(const std::string& json, const std::string& key, int defaultValue) {
    std::regex pattern("\"" + key + "\"\\s*:\\s*(-?\\d+)");
    std::smatch match;
    if (std::regex_search(json, match, pattern)) {
        return std::stoi(match[1].str());
    }
    return defaultValue;
}

/**
* @brief Extrae el valor real (double) de una clave del JSON.
*
* @param[in] json Contenido completo del fichero de configuración.
* @param[in] key Nombre de la clave, sin comillas.
* @param[in] defaultValue Valor devuelto si la clave no se encuentra.
*
* @return El número asociado a la clave, o defaultValue.
*
* @note Admite números con o sin decimales y con signo negativo (ej. -3.7038),
*       pero no notación científica (1e-3).
*/
inline double extractDouble(const std::string& json, const std::string& key, double defaultValue) {
    std::regex pattern("\"" + key + "\"\\s*:\\s*(-?\\d+\\.?\\d*)");
    std::smatch match;
    if (std::regex_search(json, match, pattern)) {
        return std::stod(match[1].str());
    }
    return defaultValue;
}

/**
* @brief Carga la configuración desde un fichero JSON.
*
* @details Parte de los valores por defecto de Config y sustituye solo los que
*          aparecen en el fichero, así que el JSON puede contener únicamente
*          las claves que se quieran cambiar.
*
* @param[in] path Ruta al fichero, relativa al directorio de trabajo
*            (normalmente "config.json", copiado junto al ejecutable por CMake).
*
* @return Configuración resultante. Si el fichero no existe, se devuelven todos
*         los valores por defecto y se muestra un aviso por consola.
*
* @warning Las claves mal escritas o con un tipo de valor incorrecto se ignoran
*          EN SILENCIO y se usa el valor por defecto, sin ningún aviso. Ante un
*          comportamiento inesperado, conviene revisar primero los nombres de
*          las claves en config.json. Tampoco se validan los valores (por
*          ejemplo, fps_video = 0 provocaría una división entre cero en
*          EdgeStreamingService).
*/
inline Config loadConfig(const std::string& path) {
    Config config;

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[WARN] No se encontro " << path << ", usando configuracion por defecto." << std::endl;
        return config;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    config.transport = extractString(json, "transporte", config.transport);
    config.groundStationHost = extractString(json, "host", config.groundStationHost);
    config.telemetryPort = extractInt(json, "puerto_telemetria", config.telemetryPort);
    config.videoPort = extractInt(json, "puerto_video", config.videoPort);
    config.modelPath = extractString(json, "modelo", config.modelPath);
    config.videoSource = extractString(json, "fuente_video", config.videoSource);
    config.videoTargetFps = extractInt(json, "fps_video", config.videoTargetFps);
    config.initialLatitude = extractDouble(json, "latitud_inicial", config.initialLatitude);
    config.initialLongitude = extractDouble(json, "longitud_inicial", config.initialLongitude);
    config.mqttTopic = extractString(json, "mqtt_topic", config.mqttTopic);

    return config;
}