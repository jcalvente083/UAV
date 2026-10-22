/**
* @file ConnectionWindow.hpp
*
* @brief Ventana inicial de la estación de tierra: configuración
*        de conexión y arranque del receptor correspondiente.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTimer>
#include <string>

#include "Ports/IReceiver.hpp"

/**
* @class ConnectionWindow
*
* @brief Primera ventana que ve el usuario: elige IP:puerto y protocolo
*        (UDP/TCP/MQTT), y al conectar da paso a DashboardWindow.
*
* @details Instancia el adaptador de IReceiver correspondiente al protocolo
*          elegido (UDPReceiver, TCPReceiver o MQTTReceiver) según el patrón
*          de fábrica simple del switch en onConnectButtonClicked(). El flujo
*          de la aplicación es ventana→ventana, al recibir el primer dato, 
*          esta ventana se oculta (no se cierra) y se muestra DashboardWindow
*          a pantalla completa.
*/
class ConnectionWindow : public QWidget {
    Q_OBJECT

public:
    /**
    * @brief Construye la ventana: formulario de IP:puerto, combo de
    *        protocolo, botón de conexión y el temporizador de timeout.
    * 
    * @details El temporizador timeoutTimer es de un solo disparo (3000 ms):
    *          si no llega deviceConnected() del receptor en ese plazo tras
    *          pulsar "Conectar al Dron", se considera fallo y se resetea la UI.
    * 
    * @param parent Objeto padre de Qt.
    */
    ConnectionWindow(QWidget* parent = nullptr);

    /**
    * @brief Destructor por defecto.
    */
    ~ConnectionWindow() = default;

    /**
    * @brief Devuelve el texto crudo del campo de IP:puerto, sin separar.
    * 
    * @return Cadena tal como la escribió el usuario (ej. "127.0.0.1:5000").
    */
    std::string getIp() const;

    /**
    * @brief Devuelve el texto crudo del protocolo seleccionado en el combo.
    * 
    * @return "UDP", "TCP" o "MQTT", según lo elegido.
    * 
    */
    std::string getConnectionType() const;


private:
    QLineEdit* ipLineEdit;         ///< Campo de texto combinado "IP:puerto" del dron/broker.
    QComboBox* conectionComboBox;  ///< Selector de protocolo: "UDP", "TCP" o "MQTT".
    QPushButton* btnConnect;       ///< Botón que dispara el intento de conexión.
    QTimer* timeoutTimer;          ///< Temporizador de un solo disparo (3000 ms) para detectar fallos de conexión.

    /**
    * @brief Receptor activo, instanciado según el protocolo elegido.
    * 
    * @details Se crea una única vez (if (receiver == nullptr) en
    *          onConnectButtonClicked()) y persiste durante toda la vida de
    *          la aplicación, no se destruye al pasar a DashboardWindow, que
    *          solo recibe un puntero, no la propiedad. Su ciclo de vida
    *          real termina cuando ConnectionWindow se destruye al final
    *          de main(), o antes si onConnectionTimeout() lo borra tras un
    *          fallo.
    */
    IReceiver* receiver;

private slots:
    /**
    * @brief Slot del botón "Conectar al Dron".
    * 
    * @details Deshabilita el formulario, arranca el timeout, instancia el
    *          receptor adecuado según el protocolo del combo (solo la primera
    *          vez), y llama a listen() con el host:puerto introducidos.
    * 
    * @warning La señal deviceConnected() del receptor se conecta antes de
    *          llamar a listen(), no después. Esto es vital para MQTTReceiver,
    *          cuyo listen() puede emitir esa señal de forma síncrona, antes de
    *          retornar, si el connect() se hiciera después, la señal se emitiría 
    *          "al vacío" y el receptor jamás recibiría el aviso, provocando un 
    *          timeout aunque la conexión al broker hubiera sido perfecta.
    */
    void onConnectButtonClicked();

    /**
    * @brief Slot del timeoutTimer: se dispara si no hay conexión en 3 segundos.
    * 
    * @details Borra el receptor a medio construir, reactiva el formulario y
    *          muestra un aviso de error al usuario.
    */
    void onConnectionTimeout();

    /**
    * @brief Slot conectado a IReceiver::deviceConnected.
    * 
    * @details Detiene el timeout, se desconecta de la señal (para no volver
    *          a dispararse), crea DashboardWindow pasándole el receiver (sin
    *          transferir su propiedad), lo muestra a pantalla completa, y
    *          oculta esta ventana.
    */
    void onFirstPacketReceived();
};