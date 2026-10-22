/**
* @file DashboardWindow.hpp
*
* @brief Ventana principal de telemetría de la estación de tierra.
*
* @author Jesús David Calvente Zapata.
*/
#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <QPixmap>
#include <QTimer>
#include <QMap>
#include <vector>

#include "Ports/IReceiver.hpp"
#include "Ports/IVideoReceiver.hpp"
#include "Domain/Common.hpp"

/**
* @class DashboardWindow
*
* @brief Pantalla completa mostrada tras conectar: telemetría, vídeo con
*        cajas de detección superpuestas, y lista de últimas detecciones.
*
* @details Recibe un IReceiver ya conectado (creado y puesto a escuchar por
*          ConnectionWindow, según el protocolo elegido: UDP/TCP/MQTT) y crea
*          además su propio UDPVideoReceiver interno para el canal de vídeo.
*          El vídeo va siempre por UDP en el puerto fijo 5001, con independencia 
*          de qué protocolo se haya elegido para la telemetría.
*
* @note Telemetría y vídeo viajan por canales completamente independientes,
*       a ritmos distintos y sin sincronización explícita entre frame y
*       detección. Las cajas dibujadas en onFrameReceived() corresponden a
*       la última detección conocida (latestDetections), que puede no
*       corresponder exactamente al frame que se está mostrando en ese
*       instante, pudiendo existir un desfase aceptado conscientemente por simplicidad
*/
class DashboardWindow : public QWidget {
    Q_OBJECT

public:
    /**
    * @brief Construye el dashboard: arranca el receptor de vídeo propio,
    *        monta la interfaz (vídeo + telemetría + detecciones), carga los
    *        nombres de clase COCO, y arranca el watchdog de red.
    * 
    * @details watchdogTimer (telemetría) se arranca inmediatamente, porque
    *          para llegar a construirse este dashboard ya tuvo que recibirse
    *          el primer paquete de telemetría. En cambio, videoWatchdogTimer 
    *          no se arranca aquí, se deja sin iniciar hasta que llega el primer 
    *          frame real en onFrameReceived(), porque el vídeo es un canal aparte 
    *          sin esa misma garantía previa; arrancarlo ya en el constructor
    *          produciría un falso "vídeo perdido" si el primer frame tarda
    *          más de 1 segundo en llegar, sin que eso implique ningún fallo
    *          real.
    * 
    * @param[in] receiver Receptor de telemetría ya construido y en escucha
    * @param[in] parent Objeto padre de Qt.
    */
    explicit DashboardWindow(IReceiver* receiver, QWidget* parent = nullptr);

    /**
    * @brief Destructor por defecto.
    */
    ~DashboardWindow() override = default;

private:
    IReceiver* networkReceiver;                ///< Receptor de telemetría (UDP/TCP/MQTT), prestado por ConnectionWindow.
    IVideoReceiver* videoReceiver;              ///< Receptor de vídeo propio (siempre UDP, puerto fijo 5001).

    std::vector<Detection> latestDetections;    ///< Última lista de detecciones recibida por telemetría; se dibuja sobre el frame de vídeo más reciente.

    QLabel* videoLabel;                         ///< Muestra el frame de vídeo decodificado con las cajas de detección superpuestas.
    QLabel* statusLabel;                         ///< Estado de la conexión de telemetría ("Conectado"/"Desconectado").
    QLabel* latencyLabel;                        ///< Latencia estimada entre envío y recepción del último paquete de telemetría.

    QLabel* batteryLabel;                        ///< Nivel de batería (%).
    QLabel* velocityLabel;                       ///< Velocidad (m/s).
    QLabel* altitudeLabel;                      ///< Altitud (m).
    QLabel* coordLabel;                         ///< Coordenadas GPS ("lat,lon").

    QListWidget* predictionListWidget;          ///< Lista de las detecciones del último paquete (clase + confianza).
    QPushButton* btnExit;                       ///< Botón que cierra la aplicación por completo.

    QTimer* watchdogTimer;                      ///< Vigila la llegada de telemetría; single-shot, 2000 ms.
    QTimer* videoWatchdogTimer;                 ///< Vigila la llegada de vídeo; single-shot, 1000 ms, arrancado de forma diferida.

    QMap<int, QString> classNames;              ///< Mapa class_id (COCO, 0-79) -> nombre en español.

    /**
    * @brief Carga el diccionario de clases COCO desde el recurso embebido.
    * 
    * @details Lee ":/coco_classes.json", un fichero JSON compilado dentro
    *          del propio binario vía el sistema de recursos de Qt (.qrc),
    *          no un fichero suelto en disco. Se llama una única vez, en el
    *          constructor, para no repetir la lectura/parseo en cada
    *          detección mostrada.
    */
    void loadClassNames();

    /**
    * @brief Parsea el JSON de telemetría recibido a la estructura de dominio.
    * 
    * @details Traduce las claves en español del contrato de protocolo
    *          (bateria, velocidad, altura, coordenadas, detecciones) a los
    *          campos en inglés de TelemetryData/Detection. coordinates se
    *          convierte a std::string porque Common.hpp es un
    *          módulo de dominio compartido y neutro respecto a Qt, pensado
    *          para poder copiarse tal cual al proyecto del dron.
    * 
    * @param[in] jsonBytes Bytes crudos del mensaje recibido.
    * 
    * @return TelemetryData con los campos disponibles; si el JSON es
    *         inválido, devuelve una estructura vacía (con timestamp 0) y
    *         registra el error en el log de depuración.
    */
    TelemetryData parseTelemetry(const QByteArray& jsonBytes);

    /**
    * @brief Actualiza todas las etiquetas de la interfaz con los datos recibidos.
    * 
    * @details La latencia se calcula como diferencia entre el reloj local y
    *          el timestamp recibido, asume relojes sincronizados entre dron
    *          y estación; esta asunción puede no cumplirse en todos los entornos,
    *          lo que puede producir una latencia negativa sin sentido. 
    * 
    * @param[in] data Telemetría ya parseada, lista para mostrarse.
    */
    void updateUI(const TelemetryData& data);

    /**
    * @brief Traduce un class_id COCO a su nombre en español.
    * 
    * @param[in] class_id Índice de clase COCO (0-79).
    * 
    * @return Nombre en español si existe en classNames; "Desconocido" en
    *         caso contrario.
    */
    QString renameClass(int class_id);

private slots:
    /**
    * @brief Slot conectado a IReceiver::dataReceived.
    * 
    * @details Extrae los bytes con networkReceiver->receive(), los ignora si
    *          vienen vacíos, reinicia el watchdog de red (marca el estado
    *          como "Conectado"), parsea el JSON, actualiza la interfaz, y
    *          guarda las detecciones en latestDetections para que
    *          onFrameReceived() las use al pintar el siguiente frame de vídeo.
    */
    void onDataReceived();

    /**
    * @brief Slot conectado a IVideoReceiver::frameReceived.
    * 
    * @details Reinicia el watchdog de vídeo, decodifica el JPEG recibido, y
    *          dibuja sobre él las cajas de latestDetections. Las coordenadas
    *          de cada detección (normalizadas 0-1) se desnormalizan multiplicando 
    *          por el ancho/alto real del pixmap decodificado, así las cajas cuadran 
    *          sea cual sea la resolución real del vídeo recibido, independientemente
    *          de a qué resolución trabaje el modelo de detección en el dron.
    * 
    * @param[in] frameData Bytes JPEG crudos de un frame completo, sin decodificar.
    */
    void onFrameReceived(const QByteArray& frameData);

    /**
    * @brief Slot del watchdog de telemetría: se dispara si no llega ningún
    *        dato en 2 segundos.
    * 
    * @details Al ser watchdogTimer de tipo single-shot, este slot solo se
    *          ejecuta una vez por cada pérdida de señal.
    */
    void onConnectionLost();

    /**
    * @brief Slot del watchdog de vídeo: se dispara si no llega ningún frame
    *        en 1 segundo desde el último recibido.
    */
    void onVideoLost();

    /**
    * @brief Slot del botón "Salir".
    * @details Detiene ambos receptores (telemetría y vídeo) de forma
    *          explícita antes de cerrar, y llama a QCoreApplication::quit()
    *          para forzar el cierre completo de la aplicación.
    */
    void onExitClicked();
};