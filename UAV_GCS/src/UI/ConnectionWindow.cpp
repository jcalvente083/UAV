#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>

#include "Networks/UDPReceiver.hpp"
#include "Networks/TCPReceiver.hpp"
#include "Networks/MQTTReceiver.hpp"
#include "UI/ConnectionWindow.hpp"
#include "UI/DashboardWindow.hpp"
#include "Domain/Protocol.hpp"


ConnectionWindow::ConnectionWindow(QWidget* parent) : QWidget(parent) {
	setWindowTitle("Estación Base UAV - Sistema B - Ventana de Inicio");
	resize(400, 300);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	QHBoxLayout* ipLayout = new QHBoxLayout();
	QLabel* ipLabel = new QLabel("IP del Dron: ");
	ipLineEdit = new QLineEdit();
	ipLineEdit->setText("127.0.0.1:5000");
	ipLayout->addWidget(ipLabel);
	ipLayout->addWidget(ipLineEdit);

	QHBoxLayout* conectionLayout = new QHBoxLayout();
	QLabel* conectionLabel = new QLabel("Tipo de conexión: ");
	conectionComboBox = new QComboBox();
	conectionComboBox->addItem("UDP");
	conectionComboBox->addItem("TCP");
	conectionComboBox->addItem("MQTT");
	conectionLayout->addWidget(conectionLabel);
	conectionLayout->addWidget(conectionComboBox);

	btnConnect = new QPushButton("Conectar al Dron");

	mainLayout->addLayout(ipLayout);
	mainLayout->addLayout(conectionLayout);
	mainLayout->addWidget(btnConnect);

	timeoutTimer = new QTimer(this);
	timeoutTimer->setSingleShot(true);

	receiver = nullptr;

	connect(btnConnect, &QPushButton::clicked, this, &ConnectionWindow::onConnectButtonClicked);
	connect(timeoutTimer, &QTimer::timeout, this, &ConnectionWindow::onConnectionTimeout);
}

void ConnectionWindow::onConnectButtonClicked() {
	btnConnect->setEnabled(false);
	btnConnect->setText("Conectando... (Espere)");
	ipLineEdit->setEnabled(false);
	conectionComboBox->setEnabled(false);

	timeoutTimer->start(3000);

	if (receiver == nullptr) {
		switch (protocolFromComboText(conectionComboBox->currentText())) {

		case Protocol::UDP:
			receiver = new UDPReceiver(this);
			break;
		case Protocol::TCP:
			receiver = new TCPReceiver(this);
			break;
		case Protocol::MQTT:
			receiver = new MQTTReceiver(this);
			break;
		default:
			timeoutTimer->stop();

			btnConnect->setEnabled(true);
			btnConnect->setText("Conectar al Dron");
			ipLineEdit->setEnabled(true);
			conectionComboBox->setEnabled(true);

			QMessageBox::information(this, "No disponible",
				"El protocolo seleccionado aún no está implementado.");
			return;

		}

		connect(receiver, &IReceiver::deviceConnected, this, &ConnectionWindow::onFirstPacketReceived);
	}

	std::string ipEscucha = ipLineEdit->text().split(":").first().toStdString();
	int puertoEscucha = ipLineEdit->text().split(":").last().toInt();

	puertoEscucha = (puertoEscucha == 0) ? 5000 : puertoEscucha;

	if (receiver->listen(ipEscucha, puertoEscucha)) {
		btnConnect->setText("Esperando al dispositivo...");
	}
	else {
		timeoutTimer->stop();

		delete receiver;
		receiver = nullptr;

		btnConnect->setEnabled(true);
		btnConnect->setText("Conectar al Dron");
		ipLineEdit->setEnabled(true);
		conectionComboBox->setEnabled(true);

		QMessageBox::critical(this, "Error de Puerto",
			"No se pudo abrir el puerto " + QString::number(puertoEscucha) + ". Puede que esté en uso.");
	}
}

void ConnectionWindow::onConnectionTimeout() {
	if (receiver != nullptr) {
		delete receiver;
		receiver = nullptr;
	}

	btnConnect->setEnabled(true);
	btnConnect->setText("¡Conectar al Dron!");
	ipLineEdit->setEnabled(true);
	conectionComboBox->setEnabled(true);

	QMessageBox::critical(this, "Error de red", "Timeout: El dron no responde en la IP especificada.\nCompruebe la conexión y vuelva a intentarlo.");
}

std::string ConnectionWindow::getIp() const {
	return ipLineEdit->text().toStdString();
}

std::string ConnectionWindow::getConnectionType() const {
	return conectionComboBox->currentText().toStdString();
}

void ConnectionWindow::onFirstPacketReceived() {
	timeoutTimer->stop();

	disconnect(receiver, &IReceiver::deviceConnected, this, &ConnectionWindow::onFirstPacketReceived);

	DashboardWindow* dashboard = new DashboardWindow(receiver);
	dashboard->setAttribute(Qt::WA_DeleteOnClose);
	dashboard->showFullScreen();

	this->hide();
}