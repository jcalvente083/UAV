#include <QHostAddress>
#include <vector>
#include <QDebug>

#include "Networks/UDPReceiver.hpp" 

UDPReceiver::UDPReceiver(QObject* parent)
	: IReceiver(parent), isFirstPacket(true) {
	socket = new QUdpSocket(this);
	connect(socket, &QUdpSocket::readyRead, this, &UDPReceiver::onReadyRead);
}


UDPReceiver::~UDPReceiver() {



}

bool UDPReceiver::listen(const std::string& host, int port) {
	return socket->bind(QHostAddress::Any, port);
}

bool UDPReceiver::isListening() const {

	return socket->state() == QAbstractSocket::BoundState;
}

void UDPReceiver::stop() {
	socket->close();
}

std::vector<uint8_t> UDPReceiver::receive() {

	std::vector<uint8_t> buffer;

	while (socket->hasPendingDatagrams()) {

		qint64 size = socket->pendingDatagramSize();
		buffer.resize(size);

		socket->readDatagram(reinterpret_cast<char*>(buffer.data()), buffer.size());

		qDebug() << "¡PAQUETE RECIBIDO DEL DRON! Tamaño:" << size << "bytes. Contenido:" << QByteArray(reinterpret_cast<char*>(buffer.data()), size);
	}
	return buffer;
}

void UDPReceiver::onReadyRead() {
	if (isFirstPacket) {
		isFirstPacket = false;
		emit deviceConnected();
	}

	emit dataReceived();
}