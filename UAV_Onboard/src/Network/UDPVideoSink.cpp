#include "Network/UDPVideoSink.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>

UDPVideoSink::UDPVideoSink(int maxHeight, int jpegQuality)
    : maxHeight(maxHeight), jpegQuality(jpegQuality) {
}

UDPVideoSink::~UDPVideoSink() {
    if (connected) {
        stop();
    }
}

bool UDPVideoSink::start(const std::string& host, int port) {
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

bool UDPVideoSink::publish(const ImageData& frame) {
    if (!connected) {
        return false;
    }

    cv::Mat img(frame.height, frame.width, CV_8UC3, const_cast<uint8_t*>(frame.data.data()));

    cv::Mat toEncode;
    if (frame.height > maxHeight) {
        float scale = static_cast<float>(maxHeight) / frame.height;
        int newW = static_cast<int>(frame.width * scale);
        cv::resize(img, toEncode, cv::Size(newW, maxHeight));
    }
    else {
        toEncode = img;
    }

    constexpr size_t MAX_UDP_SAFE_SIZE = 60000;

    if (calibratedQuality == -1) {
        calibratedQuality = jpegQuality;
        for (int attempt = 0; attempt < 4; ++attempt) {
            std::vector<int> encodeParams = { cv::IMWRITE_JPEG_QUALITY, calibratedQuality };
            encodeBuffer.clear();
            cv::imencode(".jpg", toEncode, encodeBuffer, encodeParams);
            if (encodeBuffer.size() <= MAX_UDP_SAFE_SIZE) {
                break;
            }
            calibratedQuality -= 15;
        }
        std::cout << "[INFO] Calidad JPEG calibrada a " << calibratedQuality << std::endl;
    }
    else {
        std::vector<int> encodeParams = { cv::IMWRITE_JPEG_QUALITY, calibratedQuality };
        encodeBuffer.clear();
        cv::imencode(".jpg", toEncode, encodeBuffer, encodeParams);

        if (encodeBuffer.size() > MAX_UDP_SAFE_SIZE) {
            calibratedQuality = std::max(10, calibratedQuality - 15);
            std::cerr << "[WARN] Frame no cupo, recalibrando a " << calibratedQuality << " para los siguientes." << std::endl;
            return false;
        }
    }

    int sent = send(sock, reinterpret_cast<const char*>(encodeBuffer.data()),
        static_cast<int>(encodeBuffer.size()), 0);
    return sent >= 0;
}

void UDPVideoSink::stop() {
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    connected = false;
}