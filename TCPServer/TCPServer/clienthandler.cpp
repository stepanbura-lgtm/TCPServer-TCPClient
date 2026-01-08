#include "clienthandler.h"

#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <QDataStream>
#include <QEventLoop>
#include <QThread>

constexpr quint16 MAX_BLOCK_SIZE = 0xFFFF;

ClientHandler::ClientHandler(qintptr socketDescriptor, QObject *parent)
    : QObject(parent), _socketDescriptor(socketDescriptor)
{
}

ClientHandler::~ClientHandler()
{
    if (_socket)
    {
        _socket->disconnect();
        _socket->deleteLater();
    }
}

void ClientHandler::start()
{
    _socket = new QTcpSocket(this);
    if (!_socket->setSocketDescriptor(_socketDescriptor))
    {
        qCritical() << "Failed to set socket descriptor";
        emit clientDisconnected();
        return;
    }

    qInfo() << "ClientHandler: Client connected from" << _socket->peerAddress().toString() << _socket->peerPort();

    connect(_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(_socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected);
    connect(_socket, &QTcpSocket::errorOccurred, this, &ClientHandler::onError);
}

void ClientHandler::onReadyRead()
{
    // Append available data to the per-client buffer
    _state.buffer.append(_socket->readAll());

    QDataStream in(&_state.buffer, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_5_9);

    while (true)
    {
        if (_state.buffer.size() < static_cast<int>(sizeof(quint16)))
            break;

        quint16 blockSize = 0;
        in >> blockSize;

        if (in.status() != QDataStream::Ok)
        {
            qWarning() << "DataStream read error";
            _socket->disconnectFromHost();
            return;
        }

        if (blockSize == 0)
        {
            // nothing to read; consume header and continue
            int consumed = static_cast<int>(in.device()->pos());
            _state.buffer.remove(0, consumed);
            in.device()->seek(0);
            continue;
        }

        if (blockSize > MAX_BLOCK_SIZE)
        {
            qWarning() << "Rejecting too-large block" << blockSize;
            _socket->disconnectFromHost();
            return;
        }

        // If full payload not yet received, wait for more data
        if (_state.buffer.size() - static_cast<int>(sizeof(quint16)) < static_cast<int>(blockSize))
            break;

        QString data;
        in >> data;

        if (in.status() != QDataStream::Ok)
        {
            qWarning() << "DataStream payload error";
            _socket->disconnectFromHost();
            return;
        }

        // determine how many bytes were consumed by this read
        int consumed = static_cast<int>(in.device()->pos());
        _state.buffer.remove(0, consumed);

        qInfo() << "Received:" << data;
        sendToClient(data);

        // prepare new stream view into the remaining buffer
        in.device()->seek(0);
        if (_state.buffer.isEmpty())
            break;
    }
}

void ClientHandler::emitDisconnected()
{
    if (_disconnectEmitted)
        return;
    _disconnectEmitted = true;
    emit clientDisconnected();
}

void ClientHandler::onDisconnected()
{
    qInfo() << "Client disconnected";
    emitDisconnected();
}

void ClientHandler::onError(QAbstractSocket::SocketError err)
{
    qWarning() << "Socket error" << err << "from" << _socket->peerAddress().toString();
    emitDisconnected();
}

void ClientHandler::sendToClient(const QString &data)
{
    if (!_socket || _socket->state() != QAbstractSocket::ConnectedState)
        return;

    _outBuffer.clear();
    QDataStream out(&_outBuffer, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_9);
    out << quint16(0) << data;
    out.device()->seek(0);
    out << quint16(_outBuffer.size() - sizeof(quint16));

    _socket->write(_outBuffer);
}
