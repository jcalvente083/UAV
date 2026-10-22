/**
* @file SimulatedTelemetryProvider.hpp
*
* @brief Adaptador de ITelemetryProvider que simula los datos de vuelo.
*
* @details Genera valores de telemetría plausibles sin hardware real, para poder
*          desarrollar y probar el sistema completo (dron + estación de tierra)
*          sin un controlador de vuelo.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once

#include "Ports/ITelemetryProvider.hpp"
#include "Domain/Common.hpp"

/**
* @class SimulatedTelemetryProvider
*
* @brief Adaptador (arquitectura hexagonal) que implementa ITelemetryProvider con datos sintéticos.
*
* @details Comportamiento de cada muestra:
*          - Batería: baja 0,01 puntos por muestra, sin bajar de 0.
*          - Altitud: oscila suavemente entre 45 y 55 m (50 ± 5).
*          - Velocidad: oscila entre 7 y 13 m/s (10 ± 3), nunca negativa.
*          - Posición: parte del punto inicial y describe una trayectoria cerrada,
*            aproximadamente circular, de unos 0,002° de radio (del orden de
*            200 m), volviendo periódicamente al punto de partida.
*          Altitud, velocidad y posición comparten la misma fase interna, que
*          avanza 0,05 rad por muestra (un ciclo completo cada ~126 muestras).
*
* @note El "tiempo" de la simulación avanza por MUESTRA, no por segundo real.
*       Como se toma una muestra por vuelta del bucle principal, la velocidad
*       aparente de la simulación depende de los fps configurados: a 10 fps un
*       ciclo dura ~12,6 s y la batería se agota en ~17 min; a 20 fps, la mitad
*       en ambos casos.
*/
class SimulatedTelemetryProvider : public ITelemetryProvider {
public:
    /**
    * @brief Construye el simulador en el punto de partida indicado.
    *
    * @param[in] initialLat Latitud inicial en grados (campo latitud_inicial de config.json).
    * @param[in] initialLon Longitud inicial en grados (campo longitud_inicial de config.json).
    */
    SimulatedTelemetryProvider(double initialLat = 0.0, double initialLon = 0.0);

    /**
    * @brief Genera la siguiente muestra y hace avanzar la simulación.
    *
    * @details El timestamp se toma de system_clock (hora real en ms desde
    *          epoch), necesario para que la estación de tierra pueda estimar
    *          la latencia. Las coordenadas se formatean como "lat,lon" con 6
    *          decimales (precisión del orden de 10 cm).
    *
    * @return Muestra con todos los campos de vuelo rellenos y detections vacío.
    */
    TelemetryData sampleTelemetry() override;

private:
    double batteryLevel = 100.0;    ///< Nivel de batería actual, en porcentaje.
    double time = 0.0;              ///< Fase interna de la simulación, en radianes; avanza 0,05 por muestra.
    double latitude = 0.0;          ///< Latitud actual, en grados.
    double longitude = 0.0;         ///< Longitud actual, en grados.
};