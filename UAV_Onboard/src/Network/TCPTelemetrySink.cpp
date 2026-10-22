#include "Network/TCPTelemetrySink.hpp"
#include <cstdint>

TCPTelemetrySink::TCPTelemetrySink() {}

TCPTelemetrySink::~TCPTelemetrySink() {
    if (connected) {
        stop();
    }
}

bool TCPTelemetrySink::start(const std::string& host, int port) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
#endif

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCK_T) {
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        connected = false;
        return false;
    }

    connected = true;
    return true;
}

bool TCPTelemetrySink::sendAll(const char* data, size_t length) {
    size_t totalSent = 0;
    while (totalSent < length) {
        int sent = send(sock, data + totalSent, static_cast<int>(length - totalSent), 0);
        if (sent <= 0) {
            return false;
        }
        totalSent += static_cast<size_t>(sent);
    }
    return true;
}

bool TCPTelemetrySink::publish(const std::string& json) {
    if (!connected) {
        return false;
    }

    uint32_t lengthNetworkOrder = htonl(static_cast<uint32_t>(json.size()));

    std::string packet;
    packet.reserve(sizeof(lengthNetworkOrder) + json.size());
    packet.append(reinterpret_cast<const char*>(&lengthNetworkOrder), sizeof(lengthNetworkOrder));
    packet.append(json);

    return sendAll(packet.data(), packet.size());
}

void TCPTelemetrySink::stop() {
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    connected = false;
}