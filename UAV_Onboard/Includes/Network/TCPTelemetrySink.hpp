/**
* @file TCPTelemetrySink.hpp
*
* @brief Adaptador TCP del puerto ITelemetrySink.
*
* @details Envía la telemetría por una conexión TCP hacia la estación de tierra,
*          que actúa como servidor (TCPReceiver). Usa sockets nativos (Winsock en
*          Windows, sockets POSIX en Linux) a través de SocketCompat.hpp.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include <string>
#include "Ports/ITelemetrySink.hpp"
#include "Network/SocketCompat.hpp"

/**
* @class TCPTelemetrySink
*
* @brief Adaptador (arquitectura hexagonal) que implementa ITelemetrySink sobre TCP.
*
* @details TCP es un flujo continuo de bytes, no de mensajes: si se enviaran los
*          JSON tal cual, el receptor no sabría dónde termina uno y empieza el
*          siguiente. Por eso, según el contrato de protocolo, cada mensaje va
*          precedido de su longitud en 4 bytes big-endian ([uint32][JSON]),
*          formato que TCPReceiver usa en la estación de tierra para separarlos.
*
* @note No hay reconexión automática: si la conexión se pierde, publish()
*       empieza a devolver false en cada llamada y el sink no vuelve a conectar
*       hasta que se reinicie el proceso.
*
* @warning En Linux (Docker), si la estación de tierra cierra la conexión, el
*          siguiente send() provoca la señal SIGPIPE, que por defecto termina el
*          proceso inmediatamente, sin pasar por el apagado limpio. En Windows
*          este problema no existe.
*/
class TCPTelemetrySink : public ITelemetrySink {
public:
    /**
    * @brief Construye el sink sin abrir ningún socket (eso lo hace start()).
    */
    TCPTelemetrySink();

    /**
    * @brief Destructor. Llama a stop() si el sink quedó conectado.
    */
    ~TCPTelemetrySink() override;

    /**
    * @brief Crea el socket TCP y conecta con la estación de tierra.
    *
    * @details A diferencia de UDP, connect() aquí realiza un handshake real: si
    *          nadie está escuchando en host:port en ese momento, falla. Se hace
    *          un único intento, por lo que la estación de tierra debe estar ya
    *          esperando (botón "Conectar al Dron" pulsado con TCP) ANTES de
    *          arrancar el dron.
    *
    * @param[in] host Dirección IPv4 LITERAL de la estación de tierra
    *            (ej. "192.168.1.50"). Se convierte con inet_pton, que no resuelve
    *            nombres de host.
    * @param[in] port Puerto en el que escucha la estación de tierra (por contrato, 5000).
    *
    * @retval true si la conexión TCP quedó establecida.
    * @retval false si falló WSAStartup (Windows), la creación del socket o la conexión.
    *
    * @warning El resultado de inet_pton no se comprueba: una dirección no válida
    *          se convierte en 0.0.0.0 sin ningún aviso.
    */
    bool start(const std::string& host, int port) override;

    /**
    * @brief Envía un mensaje JSON con su prefijo de longitud.
    *
    * @details Construye un único buffer [longitud de 4 bytes en big-endian][JSON]
    *          y lo envía completo con sendAll(), para hacer el menor número
    *          posible de llamadas a send().
    *
    * @param[in] json Mensaje ya serializado.
    *
    * @retval true si se enviaron todos los bytes.
    * @retval false si el sink no está conectado o el envío falló (por ejemplo,
    *         porque la estación de tierra cerró la conexión).
    *
    * @note send() es bloqueante: si la estación de tierra deja de leer y se
    *       llena el buffer TCP, esta llamada se queda esperando y, como el bucle
    *       principal es de un solo hilo, detiene también el envío de vídeo.
    */
    bool publish(const std::string& json) override;

    /**
    * @brief Cierra el socket y, en Windows, libera Winsock.
    *
    * @warning En la implementación actual debe llamarse una única vez y solo
    *          tras un start() con éxito: no comprueba si el socket es válido ni
    *          lo marca como cerrado. Si start() falla en connect(), el socket
    *          creado no se cierra.
    */
    void stop() override;

private:
    socket_t sock = INVALID_SOCK_T;     ///< Socket TCP; INVALID_SOCK_T mientras no se haya creado.
    bool connected = false;             ///< true tras un start() con éxito.

    /**
    * @brief Envía un buffer completo, aunque send() lo trocee.
    *
    * @details En TCP, send() puede enviar solo una parte de los bytes pedidos
    *          y devolver cuántos envió. Este método repite la llamada con el
    *          resto hasta completar el buffer, o hasta que send() devuelve un
    *          error o 0.
    *
    * @param[in] data Puntero al inicio del buffer.
    * @param[in] length Número de bytes a enviar.
    *
    * @retval true si se enviaron todos los bytes.
    * @retval false si send() falló antes de completar el envío.
    */
    bool sendAll(const char* data, size_t length);
};