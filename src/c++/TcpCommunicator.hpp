#ifndef TCPCOMMUNICATOR_HPP
#define TCPCOMMUNICATOR_HPP

#include <QNetworkSession>
#include <QWebSocket>

class QNetworkConfigurationManager;
class QNetworkSession;

class TcpCommunicator : public QWebSocket
{
    Q_OBJECT
public:
    enum BodyType {
        EMPTY = 0x00,
        TEXT = 0x01,
        BINARY = 0x02
    };

    enum HttpCommand {
        //Request
        SETID = 0x03,
        VERIFY_SELF = 0x04,

        //Respose
        VERIFY_PASS = 0x05,
        VERIFY_FAIL = 0x06
    };

    class HttpData {
    public:
        HttpData(){}
        quint8 fCommand;
        quint16 fId;
        quint8 fType;
        QByteArray fBody;
    };

    TcpCommunicator(QObject*);
    ~TcpCommunicator();
    void writeHttpData(TcpCommunicator::HttpData);

signals:
    void dataReceived(TcpCommunicator::HttpData);

private slots:
    void openSession();
    void configUpdated();
    void readHttpData(const QByteArray&);
    void sslErrorHandler(QList<QSslError>);
    void abstractSocketErrorHandler(QAbstractSocket::SocketError);
    void networkSessionErrorHandler(QNetworkSession::SessionError);

private:
    QNetworkConfigurationManager* fConfig_Manager;
    QNetworkSession* fNetwork_Session;
    QString fHost;
    quint16 fPort;
};

#endif // TCPCOMMUNICATOR_HPP
