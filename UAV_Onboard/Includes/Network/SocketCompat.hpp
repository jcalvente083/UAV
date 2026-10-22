/**
* @file SocketCompat.hpp
*
* @brief Capa mínima de compatibilidad de sockets entre Windows y Linux.
*
* @details Unifica los includes y tipos básicos de sockets para que los sinks de
*          red (UDPTelemetrySink, TCPTelemetrySink, UDPVideoSink) compilen sin
*          cambios tanto en Windows (Winsock, entorno de desarrollo) como en
*          Linux (sockets BSD/POSIX, contenedor Docker y emulación ARM con QEMU).
*
*          La mayoría de llamadas son idénticas en ambos sistemas, porque Winsock
*          imita la API POSIX: socket(), connect(), send(), htons(), htonl() e
*          inet_pton() se usan igual. Las diferencias reales son pocas:
*
*          | Concepto              | Windows (Winsock)        | Linux (POSIX) |
*          |-----------------------|--------------------------|---------------|
*          | Tipo de socket        | SOCKET                   | int           |
*          | Socket inválido       | INVALID_SOCKET           | -1            |
*          | Cerrar socket         | closesocket()            | close()       |
*          | Inicializar librería  | WSAStartup()/WSACleanup()| No necesario  |
*          | Librería a enlazar    | ws2_32                   | Ninguna       |
*
*          Este fichero resuelve las dos primeras filas. El cierre y la
*          inicialización de Winsock se gestionan en cada adaptador con su propio
*          bloque #ifdef _WIN32, y el enlace con ws2_32 se hace solo en Windows
*          desde CMakeLists.txt.
*
* @note Existe como fichero aparte, en lugar de repetir este bloque en cada sink,
*       porque al incluir varios sinks en la misma unidad de compilación (como
*       hace main.cpp) la constante INVALID_SOCK_T quedaba definida varias veces
*       y el compilador fallaba con un error de redefinición. Con #pragma once,
*       el bloque se procesa una sola vez aunque lo incluyan los tres sinks.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

/**
* @brief Tipo portable de descriptor de socket (en Windows, SOCKET).
*/
using socket_t = SOCKET;

/**
* @brief Valor que representa un socket no válido o no creado (en Windows, INVALID_SOCKET).
*/
constexpr socket_t INVALID_SOCK_T = INVALID_SOCKET;

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/**
* @brief Tipo portable de descriptor de socket (en Linux, un descriptor de fichero int).
*/
using socket_t = int;

/**
* @brief Valor que representa un socket no válido o no creado (en Linux, -1).
*/
constexpr socket_t INVALID_SOCK_T = -1;

#endif