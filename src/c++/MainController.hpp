#ifndef MAINCONTROLLER_HPP
#define MAINCONTROLLER_HPP

#include "TcpCommunicator.hpp"
#include <QObject>

class QQmlApplicationEngine;


class MainController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint16 clientID READ clientID NOTIFY clientIDChanged)
public:
    explicit MainController(QObject *parent = 0);
    ~MainController();
    inline quint16 clientID() const {return fClient_Id;}

signals:
    void clientIDChanged(quint16);

public slots:
    void dataReceived(TcpCommunicator::HttpData);

private:
    QQmlApplicationEngine* fUI_Engine;
    TcpCommunicator* fWeb_Socket;
    quint16 fClient_Id;
};

#endif // MAINCONTROLLER_HPP
