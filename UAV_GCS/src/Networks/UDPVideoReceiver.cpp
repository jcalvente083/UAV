#include "Networks/UDPVideoReceiver.hpp"

UDPVideoReceiver::UDPVideoReceiver(QObject* parent) : IVideoReceiver(parent) {
    socket = new QUdpSocket(this);
    connect(socket, &QUdpSocket::readyRead, this, &UDPVideoReceiver::onReadyRead);
}

UDPVideoReceiver::~UDPVideoReceiver() {
    stop();
}

bool UDPVideoReceiver::listen(int port) {
    return socket->bind(QHostAddress::Any, port);
}

void UDPVideoReceiver::stop() {
    if (socket->isOpen()) {
        socket->close();
    }
}

void UDPVideoReceiver::onReadyRead() {
    while (socket->hasPendingDatagrams()) {
        QByteArray buffer;
        buffer.resize(socket->pendingDatagramSize());

        socket->readDatagram(buffer.data(), buffer.size());

        emit frameReceived(buffer);
    }
}