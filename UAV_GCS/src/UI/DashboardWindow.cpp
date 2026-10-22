#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QJsonArray>
#include <QDebug>
#include <QDateTime>
#include <QCoreApplication>
#include <QFile>

#include "Domain/Common.hpp"
#include "ui/DashboardWindow.hpp"
#include "Networks/UDPVideoReceiver.hpp"

DashboardWindow::DashboardWindow(IReceiver* receiver, QWidget* parent)
    : QWidget(parent), networkReceiver(receiver) {

    videoReceiver = new UDPVideoReceiver(this);

    if (videoReceiver->listen(5001)) {
        qDebug() << "Escuchando vídeo en el puerto 5001";
        connect(videoReceiver, &IVideoReceiver::frameReceived, this, &DashboardWindow::onFrameReceived);
    }
    else {
        qDebug() << "Error: No se pudo abrir el puerto de vídeo";
    }

    setWindowTitle("Estación Base UAV - Sistema B - Ventana de Telemetría");

    videoLabel = new QLabel("SEÑAL DE VÍDEO PERDIDA");
    videoLabel->setAlignment(Qt::AlignCenter);

    statusLabel = new QLabel("Estado: Conectado");
    latencyLabel = new QLabel("Latencia: 0 ms");

    QVBoxLayout* leftLayout = new QVBoxLayout();
    leftLayout->addWidget(videoLabel, 1);
    leftLayout->addWidget(statusLabel);
    leftLayout->addWidget(latencyLabel);

    batteryLabel = new QLabel("Batería: 100%");
    velocityLabel = new QLabel("Velocidad: 0 km/h");
    altitudeLabel = new QLabel("Altura: 0 m");
   
    coordLabel = new QLabel("Coordenadas: 0.0, 0.0");
    
    predictionListWidget = new QListWidget();

    btnExit = new QPushButton("Salir");

    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->addWidget(batteryLabel);
    rightLayout->addWidget(velocityLabel);
    rightLayout->addWidget(altitudeLabel);
    rightLayout->addWidget(coordLabel);
    rightLayout->addSpacing(20);
    rightLayout->addWidget(new QLabel("Últimas detecciones:"));
    rightLayout->addWidget(predictionListWidget);
    rightLayout->addWidget(btnExit);

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->addLayout(leftLayout, 3);
    mainLayout->addLayout(rightLayout, 1);

    connect(btnExit, &QPushButton::clicked, this, &DashboardWindow::onExitClicked);
    connect(networkReceiver, &IReceiver::dataReceived, this, &DashboardWindow::onDataReceived);

    loadClassNames();

    this->setWindowState(Qt::WindowFullScreen);

    watchdogTimer = new QTimer(this);
    watchdogTimer->setInterval(2000); 
    watchdogTimer->setSingleShot(true);
    connect(watchdogTimer, &QTimer::timeout, this, &DashboardWindow::onConnectionLost);
    watchdogTimer->start();

    videoWatchdogTimer = new QTimer(this);
    videoWatchdogTimer->setInterval(1000);
    videoWatchdogTimer->setSingleShot(true);
    connect(videoWatchdogTimer, &QTimer::timeout, this, &DashboardWindow::onVideoLost);
    

}

void DashboardWindow::onExitClicked() {
    if (networkReceiver) {
        networkReceiver->stop();
    }
    if (videoReceiver) {
        videoReceiver->stop();
    }

    close();
    QCoreApplication::quit();
}


void DashboardWindow::onDataReceived() {
    std::vector<uint8_t> rawData = networkReceiver->receive();
    if (rawData.empty()) return;

    watchdogTimer->start(); 
    statusLabel->setText("Estado: Conectado");

    QByteArray byteArray(reinterpret_cast<const char*>(rawData.data()), rawData.size());

    TelemetryData telemetry = parseTelemetry(byteArray);

    updateUI(telemetry);

    latestDetections = telemetry.detections;
}

TelemetryData DashboardWindow::parseTelemetry(const QByteArray& jsonBytes) {
    TelemetryData data;
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonBytes, &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qDebug() << "Error parseando JSON en Dashboard:" << parseError.errorString();
        return data; 
    }

    QJsonObject jsonObj = jsonDoc.object();

    if (jsonObj.contains("timestamp")) data.timestamp = jsonObj["timestamp"].toVariant().toLongLong();
    if (jsonObj.contains("bateria")) data.battery = jsonObj["bateria"].toDouble();
    if (jsonObj.contains("velocidad")) data.velocity = jsonObj["velocidad"].toDouble();
    if (jsonObj.contains("altura")) data.altitude = jsonObj["altura"].toDouble();
    if (jsonObj.contains("coordenadas")) data.coordinates = jsonObj["coordenadas"].toString().toStdString();

    if (jsonObj.contains("detecciones")) {
        QJsonArray arrayDetecciones = jsonObj["detecciones"].toArray();
        for (const QJsonValue& valor : arrayDetecciones) {
            QJsonObject detObj = valor.toObject();
            Detection det;
            det.class_id = detObj["class_id"].toInt();
            det.confidence = detObj["confidence"].toDouble();
            det.x = detObj["x"].toDouble();
            det.y = detObj["y"].toDouble();
            det.width = detObj["width"].toDouble();
            det.height = detObj["height"].toDouble();
            data.detections.push_back(det);
        }
    }

    return data;
}

void DashboardWindow::updateUI(const TelemetryData& data) {

    if (data.timestamp > 0) {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

        qint64 latency = currentTime - data.timestamp;

        latencyLabel->setText(QString("Latencia: %1 ms").arg(latency));
    }
    else {
        latencyLabel->setText("Latencia: N/A");
    }

    batteryLabel->setText(QString("Batería: %1 %").arg(data.battery));
    velocityLabel->setText(QString("Velocidad: %1 m/s").arg(data.velocity));
    altitudeLabel->setText(QString("Altura: %1 m").arg(data.altitude));
    coordLabel->setText(QString("GPS: %1").arg(QString::fromStdString(data.coordinates)));

    predictionListWidget->clear();
    for (const Detection& det : data.detections) {
        QString textoLista = QString("Clase %1 (%2%)")
            .arg(renameClass(det.class_id))
            .arg(static_cast<int>(det.confidence * 100));

        predictionListWidget->addItem(textoLista);
    }
}


void DashboardWindow::loadClassNames() {
    QFile file(":/coco_classes.json"); 
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "[ERROR] No se pudo cargar coco_classes.json desde recursos.";
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject obj = doc.object();

    for (auto it = obj.begin(); it != obj.end(); ++it) {
        classNames[it.key().toInt()] = it.value().toString();
    }
}

QString DashboardWindow::renameClass(int class_id) {
    return classNames.value(class_id, "Desconocido");
}

void DashboardWindow::onFrameReceived(const QByteArray& frameData) {

    videoWatchdogTimer->start();

    QPixmap pixmap;

    if (pixmap.loadFromData(frameData, "JPEG")) {

        QPainter painter(&pixmap);

        QPen pen(QColor(0, 255, 0));
        pen.setWidth(3);
        painter.setPen(pen);

        QFont font = painter.font();
        font.setPixelSize(18);
        font.setBold(true);
        painter.setFont(font);

        for (const Detection& det : latestDetections) {
            int px = static_cast<int>(det.x * pixmap.width());
            int py = static_cast<int>(det.y * pixmap.height());
            int pw = static_cast<int>(det.width * pixmap.width());
            int ph = static_cast<int>(det.height * pixmap.height());

            painter.drawRect(px, py, pw, ph);

            QString label = QString("%1 %2%").arg(renameClass(det.class_id)).arg(static_cast<int>(det.confidence * 100));

            painter.fillRect(px, py - 22, painter.fontMetrics().horizontalAdvance(label) + 10, 22, QColor(0, 0, 0, 150));
            painter.setPen(QColor(255, 255, 255));
            painter.drawText(px + 5, py - 5, label);

            painter.setPen(pen);
        }

        painter.end();

        videoLabel->setPixmap(pixmap.scaled(videoLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void DashboardWindow::onConnectionLost() {
    statusLabel->setText("Estado: Desconectado");
    latencyLabel->setText("Latencia: N/A");
}

void DashboardWindow::onVideoLost() {
    videoLabel->clear();
    videoLabel->setText("SEÑAL DE VÍDEO PERDIDA");
}