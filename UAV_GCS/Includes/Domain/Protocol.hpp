/**
* @file Protocol.hpp
* 
* @brief La enumeración que representa los protocolos de comunicación disponibles.
* 
* @details Esta enumeración define los protocolos de comunicación que se pueden utilizar en el proyecto, incluyendo UDP, TCP y MQTT.
* 
* @author Jesús David Calvente Zapata.
* 
*/
#pragma once
#include <QString>

/**
* @brief La enumeración que representa los protocolos de comunicación disponibles.
* 
* @details Esta enumeración define los protocolos de comunicación que se pueden utilizar en el proyecto, los cuales son UDP, TCP y MQTT.
* 
*/
enum class Protocol {
	UDP,											///< Protocolo de comunicación UDP.
	TCP,											///< Protocolo de comunicación TCP.
	MQTT											///< Protocolo de comunicación MQTT.
};


/**
 * @brief Convierte un texto de la interfaz gráfica en un protocolo de comunicación.
 *
 * @details Evalúa una cadena de texto (procedente del QComboBox de la ventana de Conexión) y la
 *          mapea con su enumerado correspondiente.
 *
 * @param[in] text El texto que representa el protocolo
 *
 * @retval Protocol::TCP  Si el texto es exactamente "TCP".
 * @retval Protocol::MQTT Si el texto es exactamente "MQTT".
 * @retval Protocol::UDP  Si el texto es "UDP" o cualquier otro valor no reconocido (valor por defecto).
 *
 * @note La comparación distingue entre mayúsculas y minúsculas (case-sensitive).
 *       Un texto como "tcp" en minúsculas NO se reconoce como Protocol::TCP y
 *       cae erróneamente al valor por defecto (Protocol::UDP).
 */
inline Protocol protocolFromComboText(const QString& text) {

	if (text == "TCP") return Protocol::TCP;
	else if (text == "MQTT") return Protocol::MQTT;

	return Protocol::UDP;

}