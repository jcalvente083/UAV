#include "Networks/MqttReceiver.hpp"
#include <QMetaObject>

MQTTReceiver::MQTTReceiver(QObject* parent, std::string topic)
    : IReceiver(parent), topic(std::move(topic)) {
    mosquitto_lib_init();
}

MQTTReceiver::~MQTTReceiver() {
    stop();
    mosquitto_lib_cleanup();
}

#include <QDebug>

bool MQTTReceiver::listen(const std::string& host, int port) {
    mosq = mosquitto_new(nullptr, true, this);
    if (!mosq) {
        qDebug() << "[MQTT] mosquitto_new fallo";
        return false;
    }

    mosquitto_message_callback_set(mosq, &MQTTReceiver::onMessageCallback);

    int rc = mosquitto_connect(mosq, host.c_str(), port, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        qDebug() << "[MQTT] mosquitto_connect fallo, codigo:" << rc
            << "(" << mosquitto_strerror(rc) << ")";
        mosquitto_destroy(mosq);
        mosq = nullptr;
        return false;
    }

    rc = mosquitto_subscribe(mosq, nullptr, topic.c_str(), 1);
    if (rc != MOSQ_ERR_SUCCESS) {
        qDebug() << "[MQTT] mosquitto_subscribe fallo, codigo:" << rc
            << "(" << mosquitto_strerror(rc) << ")";
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
        mosq = nullptr;
        return false;
    }

    rc = mosquitto_loop_start(mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
        qDebug() << "[MQTT] mosquitto_loop_start fallo, codigo:" << rc
            << "(" << mosquitto_strerror(rc) << ")";
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
        mosq = nullptr;
        return false;
    }

    qDebug() << "[MQTT] Conectado y suscrito correctamente a" << QString::fromStdString(topic);
    connected = true;
    emit deviceConnected();
    return true;
}

bool MQTTReceiver::isListening() const {
    return connected;
}

void MQTTReceiver::stop() {
    if (mosq) {
        mosquitto_loop_stop(mosq, true);
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
        mosq = nullptr;
    }
    connected = false;
}

std::vector<uint8_t> MQTTReceiver::receive() {
    return std::vector<uint8_t>(lastMessage.begin(), lastMessage.end());
}

void MQTTReceiver::onMessageCallback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg) {
    MQTTReceiver* self = static_cast<MQTTReceiver*>(obj);
    QByteArray payload(static_cast<const char*>(msg->payload), msg->payloadlen);

    QMetaObject::invokeMethod(self, "handleMessage", Qt::QueuedConnection, Q_ARG(QByteArray, payload));
}

void MQTTReceiver::handleMessage(const QByteArray& payload) {
    lastMessage = payload;
    emit dataReceived();
}