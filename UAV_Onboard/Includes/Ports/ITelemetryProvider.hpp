/**
* @file ITelemetryProvider.hpp
*
* @brief Puerto para la obtención de la telemetría del dron.
*
* @details Define la interfaz que debe implementar cualquier fuente de datos de
*          vuelo. Actualmente la implementa SimulatedTelemetryProvider, que
*          genera valores plausibles sin hardware real.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once

#include "Domain/Common.hpp"

/**
* @class ITelemetryProvider
*
* @brief Puerto (arquitectura hexagonal) para muestrear el estado del vehículo.
*
* @details Desacopla el origen de los datos de vuelo del resto del sistema: hoy
*          es una simulación, pero un adaptador que leyera de un controlador de
*          vuelo real (por ejemplo, vía MAVLink) podría sustituirla sin tocar
*          EdgeStreamingService.
*/
class ITelemetryProvider {

public:
	/**
	* @brief Destructor virtual.
	*
	* @details Garantiza que se llame al destructor del adaptador concreto al
	*          destruirlo a través de un puntero a ITelemetryProvider.
	*/
	virtual ~ITelemetryProvider() = default;

	/**
	* @brief Toma una muestra del estado actual del vehículo.
	*
	* @details Rellena timestamp, battery, velocity, altitude y coordinates,
	*          pero deja detections VACÍO: las detecciones no son responsabilidad
	*          de este puerto, las añade EdgeStreamingService antes de enviar.
	*          El timestamp debe ser hora de reloj real (epoch en ms, no un
	*          reloj monótono), porque la estación de tierra lo compara con su
	*          propia hora para estimar la latencia.
	*
	* @note No es una consulta pura: cada llamada puede hacer avanzar el estado
	*       interno del proveedor (en la simulación, la batería baja y la
	*       posición se desplaza en cada muestra). Se invoca una vez por vuelta
	*       del bucle principal, así que el ritmo de muestreo coincide con los
	*       fps de vídeo configurados.
	*
	* @return Muestra de telemetría con todos los campos de vuelo rellenos y
	*         detections vacío.
	*/
	virtual TelemetryData sampleTelemetry() = 0;
};