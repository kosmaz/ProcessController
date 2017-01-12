#include "MainController.hpp"
#include <QQmlApplicationEngine>
#include <QQmlContext>


MainController::MainController(QObject* parent) :
    QObject(parent),
    fUI_Engine(new QQmlApplicationEngine(this)),
    fWeb_Socket(new TcpCommunicator(this))
{
    connect(fWeb_Socket, SIGNAL(dataReceived(TcpCommunicator::HttpData)), this, SLOT(dataReceived(TcpCommunicator::HttpData)));
    fUI_Engine->rootContext()->setContextProperty("MainController", this);
    fUI_Engine->load(QUrl(QStringLiteral("qrc:/main.qml")));
}

MainController::~MainController()
{
    delete fWeb_Socket;
}


void MainController::dataReceived(TcpCommunicator::HttpData)
{
    return;
}
