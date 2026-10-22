#include "Network/MQTTTelemetrySink.hpp"
#include <iostream>

namespace {
    constexpr int QOS = 1;
    constexpr int KEEPALIVE_SECONDS = 60;
}

MQTTTelemetrySink::MQTTTelemetrySink(std::string topic) : topic(std::move(topic)) {
    mosquitto_lib_init();
}

MQTTTelemetrySink::~MQTTTelemetrySink() {
    if (connected) {
        stop();
    }
    mosquitto_lib_cleanup();
}

bool MQTTTelemetrySink::start(const std::string& host, int port) {
    mosq = mosquitto_new(nullptr, true, nullptr);
    if (!mosq) {
        return false;
    }

    if (mosquitto_connect(mosq, host.c_str(), port, KEEPALIVE_SECONDS) != MOSQ_ERR_SUCCESS) {
        std::cerr << "[ERROR] No se pudo conectar al broker MQTT en " << host << ":" << port << std::endl;
        mosquitto_destroy(mosq);
        mosq = nullptr;
        return false;
    }

    if (mosquitto_loop_start(mosq) != MOSQ_ERR_SUCCESS) {
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
        mosq = nullptr;
        return false;
    }

    connected = true;
    return true;
}

bool MQTTTelemetrySink::publish(const std::string& json) {
    if (!connected) {
        return false;
    }

    int result = mosquitto_publish(mosq, nullptr, topic.c_str(),
        static_cast<int>(json.size()), json.c_str(),
        QOS, /*retain=*/false);
    return result == MOSQ_ERR_SUCCESS;
}

void MQTTTelemetrySink::stop() {
    if (mosq) {
        mosquitto_loop_stop(mosq, /*force=*/true);
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
        mosq = nullptr;
    }
    connected = false;
}