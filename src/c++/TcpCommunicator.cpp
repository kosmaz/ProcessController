#include "TcpCommunicator.hpp"
#include <QNetworkConfigurationManager>
#include <QNetworkConfiguration>
#include <QSslConfiguration>
#include <QNetworkSession>
#include <QNetworkProxy>
#include <QMessageBox>
#include <QDataStream>
#include <QByteArray>
#include <QSslKey>
#include <QFile>

TcpCommunicator::TcpCommunicator(QObject* parent) :
    QWebSocket("self-client", QWebSocketProtocol::VersionLatest, parent),
    fConfig_Manager(new QNetworkConfigurationManager(this)),
    fNetwork_Session(nullptr),
    fHost("localhost"),
    fPort(48004)
{
    QSslConfiguration ssl_config;
    ssl_config.setProtocol(QSsl::SecureProtocols);
    ssl_config.setSslOption(QSsl::SslOptionDisableCompression, true);
    ssl_config.setSslOption(QSsl::SslOptionDisableEmptyFragments, false);
    ssl_config.setSslOption(QSsl::SslOptionDisableLegacyRenegotiation, true);
    ssl_config.setSslOption(QSsl::SslOptionDisableServerNameIndication, false);
    ssl_config.setSslOption(QSsl::SslOptionDisableSessionPersistence, true);
    ssl_config.setSslOption(QSsl::SslOptionDisableSessionSharing, false);
    ssl_config.setSslOption(QSsl::SslOptionDisableSessionTickets, false);
    ssl_config.setPeerVerifyDepth(0);
    ssl_config.setPeerVerifyMode(QSslSocket::VerifyPeer);

    QFile ssl_certificate_file("SSL/WebAppController.crt");
    ssl_certificate_file.open(QIODevice::ReadOnly);
    ssl_config.setLocalCertificate(QSslCertificate(&ssl_certificate_file));
    QFile ssl_privatekey("SSL/WebAppController.key");
    ssl_privatekey.open(QIODevice::ReadOnly);
    ssl_config.setPrivateKey(QSslKey(&ssl_privatekey,QSsl::Rsa));

    setSslConfiguration(ssl_config);
    setProxy(QNetworkProxy::NoProxy);

    if(fConfig_Manager->capabilities() & QNetworkConfigurationManager::NetworkSessionRequired)
    {
        connect(fConfig_Manager, &QNetworkConfigurationManager::updateCompleted, this, &TcpCommunicator::configUpdated);
        fConfig_Manager->updateConfigurations();
    }
    else
        openSession();

}


TcpCommunicator::~TcpCommunicator()
{
    if(fNetwork_Session)
    {
        fNetwork_Session->close();
        delete fNetwork_Session;
    }
    close();
    delete fConfig_Manager;

}


void TcpCommunicator::writeHttpData(TcpCommunicator::HttpData data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << data.fCommand << data.fId << data.fType << data.fBody;
    block = qCompress(block);

    /* ping the server socket first and wait for a corresponding
     * pong from the server before sending the payload to the
     * server. This is to avoid sending data to an already disconnected
     * server.
     */
    ping();
    connect(this, &TcpCommunicator::pong, this, [=](const quint64&, const QByteArray&)
    {
        qint64 sent= sendBinaryMessage(block);
        if((qint64)block.size() != sent)
        {
            QString msg("Block Size: " + QString::number(block.size()) + "\nNumber of bytes transmitted: " + QString::number(sent));
            QMessageBox::warning(0, tr("Incomplete transmission"), tr(msg.toStdString().c_str()));
        }
    });
    return;
}


void TcpCommunicator::openSession()
{
    QFile ssl_certificate_file("SSL/WebAppController.crt");
    ssl_certificate_file.open(QIODevice::ReadOnly);
    ignoreSslErrors(QList<QSslError>({QSslError(QSslError::SelfSignedCertificate, QSslCertificate(&ssl_certificate_file))}));

    connect(this, &TcpCommunicator::binaryMessageReceived, this, &TcpCommunicator::readHttpData);
    QUrl url;
    url.setScheme("wss");
    url.setHost(fHost);
    url.setPort(fPort);
    open(url);
    connect(this, SIGNAL(sslErrors(QList<QSslError>)), SLOT(sslErrorHandler(QList<QSslError>)));
    connect(this, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(abstractSocketErrorHandler(QAbstractSocket::SocketError)));
    return;
}


void TcpCommunicator::configUpdated()
{
    QNetworkConfiguration config = fConfig_Manager->defaultConfiguration();
    if(config.isValid())
    {
        fNetwork_Session = new QNetworkSession(config, this);
        connect(fNetwork_Session, &QNetworkSession::opened, this, &TcpCommunicator::openSession);
        connect(fNetwork_Session, SIGNAL(error(QNetworkSession::SessionError)), this,
                SLOT(networkSessionErrorHandler(QNetworkSession::SessionError)));
        fNetwork_Session->open();
        connect(fNetwork_Session, &QNetworkSession::preferredConfigurationChanged, fNetwork_Session,
                [=](const QNetworkConfiguration&, const bool&){fNetwork_Session->ignore();});
    }
    else
        fConfig_Manager->updateConfigurations();
    return;
}


void TcpCommunicator::readHttpData(const QByteArray& block)
{
    QByteArray temp_block = qUncompress(block);
    QDataStream in(&temp_block, QIODevice::ReadOnly);
    TcpCommunicator::HttpData data;
    in >> data.fCommand >> data.fId >> data.fType >> data.fBody;
    emit dataReceived(data);
    return;
}


void TcpCommunicator::sslErrorHandler(QList<QSslError> error_list)
{
    if(error_list.size())
        foreach(QSslError error, error_list)
            //FIXME: use a QML file dialog here
            QMessageBox::warning(0, tr("SSL handshake error"), tr(error.errorString().toStdString().c_str()));
    return;
}


void TcpCommunicator::abstractSocketErrorHandler(QAbstractSocket::SocketError error)
{
    switch(error)
    {
        case QAbstractSocket::RemoteHostClosedError:
        case QAbstractSocket::ConnectionRefusedError: {
            //FIXME: use a QML file dialog here
            QMessageBox::critical(0, tr("Socket Error"), tr(errorString().toStdString().c_str()));
            close();
            openSession();
            break;
        }


        case QAbstractSocket::UnsupportedSocketOperationError:
        case QAbstractSocket::UnknownSocketError:
        case QAbstractSocket::SocketTimeoutError:
        case QAbstractSocket::TemporaryError:
        case QAbstractSocket::OperationError: {
            //FIXME: use a QML file dialog here
            QMessageBox::warning(0, tr("Socket Error"), tr(errorString().toStdString().c_str()));
            break;
        }


        default: {
            //FIXME: use a QML file dialog here
            QMessageBox::critical(0, tr("Socket Error"), tr(errorString().toStdString().c_str()));
            close();
            if(fConfig_Manager->capabilities() & QNetworkConfigurationManager::NetworkSessionRequired)
            {
                fNetwork_Session->close();
                delete fNetwork_Session;
                fConfig_Manager->updateConfigurations();
            }
            else
                openSession();
            break;
        }
    }

    return;
}


void TcpCommunicator::networkSessionErrorHandler(QNetworkSession::SessionError error)
{
    switch(error)
    {
        case QNetworkSession::RoamingError: {
            //FIXME: use a QML file dialog here
            QMessageBox::warning(0, tr("Network Session Error"), tr(fNetwork_Session->errorString().toStdString().c_str()));
            break;
        }

        case QNetworkSession::UnknownSessionError:
        case QNetworkSession::InvalidConfigurationError:
        case QNetworkSession::OperationNotSupportedError: {
            //FIXME: use a QML file dialog here
            QMessageBox::warning(0, tr("Network Session Error"), tr(fNetwork_Session->errorString().toStdString().c_str()));
            close();
            if(fConfig_Manager->capabilities() & QNetworkConfigurationManager::NetworkSessionRequired)
            {
                fNetwork_Session->close();
                delete fNetwork_Session;
                fConfig_Manager->updateConfigurations();
            }
            else
                openSession();
            break;
        }

        case QNetworkSession::SessionAbortedError: {
            //FIXME: use a QML file dialog here
            QMessageBox::critical(0, tr("Network Session Error"), tr(fNetwork_Session->errorString().toStdString().c_str()));
            close();
            if(fConfig_Manager->capabilities() & QNetworkConfigurationManager::NetworkSessionRequired)
            {
                fNetwork_Session->close();
                delete fNetwork_Session;
                fConfig_Manager->updateConfigurations();
            }
            else
                openSession();
            break;
        }
    }

    return;
}
