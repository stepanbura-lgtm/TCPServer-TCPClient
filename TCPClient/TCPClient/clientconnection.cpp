#include "clientconnection.h"
#include <QDataStream>
#include <QBuffer>
#include <QDebug>

constexpr quint16 MAX_PAYLOAD = 4096;
constexpr quint16 PORT = 4000;

ClientConnection::ClientConnection(QObject *parent)
    : QObject(parent)
    , _socket(new QTcpSocket(this))
    , _blockSize(0)
    , _serverIp("127.0.0.1")
    , _serverPort(PORT)
{
    connect(_socket, &QTcpSocket::readyRead, this, &ClientConnection::onReadyRead);
    connect(_socket, &QTcpSocket::disconnected, this, &ClientConnection::onDisconnected);
    connect(_socket, qOverload<QAbstractSocket::SocketError>(&QAbstractSocket::errorOccurred),
            this, &ClientConnection::onError);
    connect(_socket, &QTcpSocket::connected, this, &ClientConnection::connected);
}

ClientConnection::~ClientConnection()
{
    if (_socket)
    {
        _socket->disconnectFromHost();
        _socket->deleteLater();
    }
}

void ClientConnection::setServer(const QString &ip, quint16 port)
{
    _serverIp = ip;
    _serverPort = port;
}

void ClientConnection::connectToHost()
{
    if (!_socket)
        return;
    if (_socket->state() == QTcpSocket::ConnectedState)
        return;
    _socket->connectToHost(_serverIp, _serverPort);
}

bool ClientConnection::isConnected() const
{
    return _socket && _socket->state() == QTcpSocket::ConnectedState;
}

void ClientConnection::sendPayload(const QString &payload)
{
    if (!_socket || _socket->state() != QTcpSocket::ConnectedState)
        return;

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_9);

    // Reserve space for block size
    out << quint16(0);
    out << payload;

    quint16 blockSize = static_cast<quint16>(data.size() - sizeof(quint16));

    out.device()->seek(0);
    out << blockSize;

    _socket->write(data);
}

void ClientConnection::onReadyRead()
{
    if (!_socket)
        return;

    QDataStream in(_socket);
    in.setVersion(QDataStream::Qt_5_9);

    while (true)
    {
        if (_blockSize == 0)
        {
            if (_socket->bytesAvailable() < static_cast<int>(sizeof(quint16)))
                break;
            in >> _blockSize;

            if (in.status() != QDataStream::Ok)
            {
                if (_socket)
                    _socket->abort();
                _blockSize = 0;
                emit errorOccurred(tr("read error (header)"));
                break;
            }

            if (_blockSize > MAX_PAYLOAD)
            {
                if (_socket)
                    _socket->abort();
                _blockSize = 0;
                emit errorOccurred(tr("Incoming payload too large"));
                break;
            }
        }

        if (_socket->bytesAvailable() < _blockSize)
            break;

        QString str;
        in >> str;

        if (in.status() != QDataStream::Ok)
        {
            if (_socket)
                _socket->abort();
            _blockSize = 0;
            emit errorOccurred(tr("read error (payload)"));
            break;
        }

        emit messageReceived(str);
        _blockSize = 0;
    }
}

void ClientConnection::onDisconnected()
{
    _blockSize = 0;
    emit disconnected();
}

void ClientConnection::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    if (!_socket)
        return;
    emit errorOccurred(_socket->errorString());
}
