#include "Networks/TCPReceiver.hpp"

TCPReceiver::TCPReceiver(QObject* parent)
    : IReceiver(parent), server(new QTcpServer(this)), clientSocket(nullptr) {
    connect(server, &QTcpServer::newConnection, this, &TCPReceiver::onNewConnection);
}

TCPReceiver::~TCPReceiver() {
    stop();
}

bool TCPReceiver::listen(const std::string& host, int port) {
    return server->listen(QHostAddress::Any, port);
}

bool TCPReceiver::isListening() const {
    return server->isListening();
}

void TCPReceiver::stop() {
    if (clientSocket) {
        clientSocket->disconnect();
        clientSocket->close();
        clientSocket->deleteLater();
        clientSocket = nullptr;
    }
    server->close();
}

void TCPReceiver::onNewConnection() {
    QTcpSocket* incoming = server->nextPendingConnection();

    if (clientSocket != nullptr) {
        incoming->close();
        incoming->deleteLater();
        return;
    }

    clientSocket = incoming;
    connect(clientSocket, &QTcpSocket::readyRead, this, &TCPReceiver::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &TCPReceiver::onClientDisconnected);

    emit deviceConnected();
}

void TCPReceiver::onClientDisconnected() {
    if (clientSocket) {
        clientSocket->deleteLater();
        clientSocket = nullptr;
    }
    recvBuffer.clear();
}

void TCPReceiver::onReadyRead() {
    recvBuffer.append(clientSocket->readAll());

    bool gotMessage = false;

    while (recvBuffer.size() >= 4) {
        quint32 length =
            (static_cast<uint8_t>(recvBuffer[0]) << 24)
            | (static_cast<uint8_t>(recvBuffer[1]) << 16)
            | (static_cast<uint8_t>(recvBuffer[2]) << 8)
            | static_cast<uint8_t>(recvBuffer[3]);

        if (recvBuffer.size() < static_cast<int>(4 + length)) {
            break; 
        }

        lastMessage = recvBuffer.mid(4, length);
        recvBuffer.remove(0, 4 + length);
        gotMessage = true;
    }

    if (gotMessage) {
        emit dataReceived();
    }
}

std::vector<uint8_t> TCPReceiver::receive() {
    return std::vector<uint8_t>(
        reinterpret_cast<const uint8_t*>(lastMessage.constData()),
        reinterpret_cast<const uint8_t*>(lastMessage.constData()) + lastMessage.size()
    );
}