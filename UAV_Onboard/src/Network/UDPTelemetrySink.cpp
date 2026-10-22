#include "Network/UDPTelemetrySink.hpp"
#include <cstring>

UDPTelemetrySink::UDPTelemetrySink() {}

UDPTelemetrySink::~UDPTelemetrySink() {
    if (connected) {
        stop();
    }
}

bool UDPTelemetrySink::start(const std::string& host, int port) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
#endif

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCK_T) {
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        return false;
    }

    connected = true;
    return true;
}

bool UDPTelemetrySink::publish(const std::string& json) {
    if (!connected) {
        return false;
    }
    return send(sock, json.c_str(), static_cast<int>(json.size()), 0) >= 0;
}

void UDPTelemetrySink::stop() {
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    connected = false;
}