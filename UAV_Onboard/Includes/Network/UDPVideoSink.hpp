/**
* @file UDPVideoSink.hpp
*
* @brief Adaptador UDP del puerto IVideoSink: envía el vídeo como JPEG sobre UDP.
*
* @details Cada frame se reescala, se comprime a JPEG y se envía en un único
*          datagrama UDP hacia la estación de tierra (UDPVideoReceiver, puerto
*          5001), según el contrato de protocolo.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include <string>
#include <vector>
#include <cstdint>

#include "Ports/IVideoSink.hpp"
#include "Network/SocketCompat.hpp"

/**
* @class UDPVideoSink
*
* @brief Adaptador (arquitectura hexagonal) que implementa IVideoSink con JPEG sobre UDP.
*
* @details Regla de transporte: un datagrama = un frame JPEG completo, sin
*          fragmentación. Un datagrama UDP admite como máximo unos 65 507 bytes
*          de datos, así que cada JPEG debe quedar por debajo de un límite de
*          seguridad de 60 000 bytes.
*
*          Para cumplirlo con calidad JPEG estable, se sigue esta estrategia:
*          1. Calibración (solo en el primer frame): se prueba la calidad
*             inicial y, si el JPEG no cabe, se baja de 15 en 15 hasta 4 intentos.
*             La calidad resultante queda fijada.
*          2. Resto del vídeo: un único encode por frame con la calidad fijada.
*             Así el coste por frame es constante. Una versión anterior
*             recalculaba la calidad en cada frame, y el número variable de
*             encodes hacía que el vídeo llegara a trompicones.
*          3. Si más adelante un frame no cabe (la escena se vuelve más
*             compleja), ese frame se descarta y la calidad se baja 15 puntos
*             de forma permanente, con un mínimo de 10. No se reintenta el
*             mismo frame, para no romper el ritmo del bucle.
*
* @note La calidad solo baja, nunca vuelve a subir aunque la escena se
*       simplifique. Si llegara al mínimo (10) y aun así los frames no cupieran,
*       se descartarían todos.
*
* @warning En la calibración inicial la calidad no tiene límite inferior (puede
*          quedar por debajo de 10) y, al terminarla, no se vuelve a comprobar el
*          tamaño antes de enviar el primer frame.
*/
class UDPVideoSink : public IVideoSink {
public:
    /**
    * @brief Construye el sink con los parámetros de compresión.
    *
    * @param[in] maxHeight Altura máxima del vídeo enviado, en píxeles. Los frames
    *            más altos se reducen manteniendo la proporción (contrato: 480p).
    * @param[in] jpegQuality Calidad JPEG inicial (0-100) desde la que empieza la calibración.
    */
    UDPVideoSink(int maxHeight = 480, int jpegQuality = 55);

    /**
    * @brief Destructor. Llama a stop() si el sink quedó conectado.
    */
    ~UDPVideoSink() override;

    /**
    * @brief Crea el socket UDP y fija la dirección de destino.
    *
    * @details Igual que en UDPTelemetrySink: connect() sobre UDP solo fija el
    *          destino por defecto para poder usar send(); no comprueba que haya
    *          nadie escuchando.
    *
    * @param[in] host Dirección IPv4 LITERAL de la estación de tierra (se convierte
    *            con inet_pton, que no resuelve nombres de host).
    * @param[in] port Puerto de vídeo de la estación de tierra (por contrato, 5001).
    *
    * @retval true si el socket quedó creado y con destino fijado.
    * @retval false si falló WSAStartup (Windows), la creación del socket o connect().
    *
    * @warning El resultado de inet_pton no se comprueba: una dirección no válida
    *          se convierte en 0.0.0.0 sin ningún aviso.
    */
    bool start(const std::string& host, int port) override;

    /**
    * @brief Reescala, comprime y envía un frame.
    *
    * @param[in] frame Frame sin comprimir en BGR. Se asume de 3 canales.
    *
    * @retval true si el datagrama se envió.
    * @retval false si el sink no está iniciado, el frame se descartó por no
    *         caber en un datagrama, o send() falló. Perder un frame puntual es
    *         aceptable y el bucle continúa con el siguiente.
    */
    bool publish(const ImageData& frame) override;

    /**
    * @brief Cierra el socket y, en Windows, libera Winsock.
    *
    * @warning En la implementación actual debe llamarse una única vez y solo
    *          tras un start() con éxito: no comprueba si el socket es válido ni
    *          lo marca como cerrado.
    */
    void stop() override;

private:
    socket_t sock = INVALID_SOCK_T;     ///< Socket UDP; INVALID_SOCK_T mientras no se haya creado.
    bool connected = false;             ///< true tras un start() con éxito.

    int maxHeight;                      ///< Altura máxima de los frames enviados, en píxeles.
    int jpegQuality;                    ///< Calidad JPEG inicial para la calibración.

    int calibratedQuality = -1;         ///< Calidad JPEG en uso; -1 significa "aún sin calibrar" (se calibra en el primer frame).

    std::vector<uint8_t> encodeBuffer;  ///< Buffer del JPEG codificado, reutilizado entre frames para no reservar memoria en cada uno.
};