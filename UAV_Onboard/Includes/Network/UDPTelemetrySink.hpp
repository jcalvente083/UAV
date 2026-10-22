/**
* @file UDPTelemetrySink.hpp
*
* @brief Adaptador UDP del puerto ITelemetrySink.
*
* @details Envía cada mensaje JSON de telemetría como un único datagrama UDP
*          hacia la estación de tierra. Usa sockets nativos (Winsock en Windows,
*          sockets POSIX en Linux) a través de SocketCompat.hpp, sin depender de
*          ninguna librería externa.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include <string>
#include "Ports/ITelemetrySink.hpp"
#include "Network/SocketCompat.hpp"

/**
* @class UDPTelemetrySink
*
* @brief Adaptador (arquitectura hexagonal) que implementa ITelemetrySink sobre UDP.
*
* @details Framing según el contrato de protocolo: un datagrama = un objeto JSON,
*          sin cabeceras adicionales. UDP no garantiza entrega ni orden, lo cual
*          es aceptable para telemetría: si se pierde un paquete, el siguiente
*          trae el estado más reciente.
*/
class UDPTelemetrySink : public ITelemetrySink {
public:
	/**
	* @brief Construye el sink sin abrir ningún socket (eso lo hace start()).
	*/
	UDPTelemetrySink();

	/**
	* @brief Destructor. Llama a stop() si el sink quedó conectado.
	*/
	~UDPTelemetrySink() override;

	/**
	* @brief Crea el socket UDP y fija la dirección de destino.
	*
	* @details Llama a connect() sobre el socket UDP. En UDP esto no establece
	*          ninguna conexión real ni envía nada: solo fija un destino por
	*          defecto, para poder usar send() en lugar de sendto() en cada
	*          mensaje. Por eso start() devuelve true aunque la estación de tierra
	*          todavía no esté escuchando.
	*
	* @param[in] host Dirección IPv4 LITERAL de la estación de tierra
	*            (ej. "192.168.1.50"). La conversión se hace con inet_pton, que no
	*            resuelve nombres: un nombre de host como "host.docker.internal"
	*            no es válido aquí.
	* @param[in] port Puerto de destino (por contrato, 5000).
	*
	* @retval true si el socket quedó creado y con destino fijado.
	* @retval false si falló WSAStartup (Windows), la creación del socket o connect().
	*
	* @warning El resultado de inet_pton no se comprueba: si host no es una IPv4
	*          válida, la dirección queda como 0.0.0.0 y en Linux los datagramas
	*          acaban enviándose a la propia máquina (dentro de Docker, al propio
	*          contenedor), sin ningún error visible.
	*/
	bool start(const std::string& host, int port) override;

	/**
	* @brief Envía un mensaje JSON en un único datagrama.
	*
	* @param[in] json Mensaje ya serializado. Debe caber en un datagrama UDP
	*            (holgadamente, ya que la telemetría ocupa unos cientos de bytes).
	*
	* @retval true si send() no devolvió error. En UDP esto solo indica que el
	*         datagrama salió de la máquina, no que llegara a su destino.
	* @retval false si el sink no está iniciado o send() falló.
	*/
	bool publish(const std::string& json) override;

	/**
	* @brief Cierra el socket y, en Windows, libera Winsock.
	*
	* @warning En la implementación actual, stop() debe llamarse una única vez y
	*          solo tras un start() con éxito: no comprueba si el socket es válido
	*          ni lo marca como cerrado, de modo que una segunda llamada volvería a
	*          cerrar el mismo descriptor. El destructor lo respeta comprobando
	*          el flag connected antes de llamarlo.
	*/
	void stop() override;

private:
	socket_t sock = INVALID_SOCK_T;	///< Socket UDP; INVALID_SOCK_T mientras no se haya creado.
	bool connected = false;			///< true tras un start() con éxito; publish() no envía si es false.
};