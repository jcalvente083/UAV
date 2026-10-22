/**
* @file main.cpp
*
* @brief Punto de entrada de la estación de tierra.
*
* @details Composition root de la aplicación: instancia la QApplication y la
*          ventana inicial (ConnectionWindow), y cede el control al bucle de
*          eventos de Qt. Toda la lógica de negocio (elección de protocolo,
*          creación de receptores, transición al dashboard) vive dentro de
*          ConnectionWindow y DashboardWindow, este fichero no contiene
*          ninguna decisión de la aplicación, solo el arranque.
*
* @author Jesús David Calvente Zapata.
*/
#include <QApplication>
#include "ui/ConnectionWindow.hpp" 

/**
* @brief Función principal.
* 
* @param[in] argc Número de argumentos de línea de comandos.
* @param[in] argv Argumentos de línea de comandos.
*
* @return Código de salida del bucle de eventos de Qt (app.exec()).
*/
int main(int argc, char* argv[]) {
	QApplication app(argc, argv);

	ConnectionWindow window;
	window.show();

	return app.exec();
}
