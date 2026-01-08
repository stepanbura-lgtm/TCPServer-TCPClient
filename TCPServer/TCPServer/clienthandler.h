#pragma once

#include <QtCore>
#include <QtNetwork/QTcpSocket>

class ClientHandler : public QObject
{
    Q_OBJECT
public:
    explicit ClientHandler(qintptr socketDescriptor, QObject *parent = nullptr);
    ~ClientHandler();

    void start();

signals:
    void clientDisconnected();

private slots:
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError err);

private:
    void sendToClient(const QString &data);
    void emitDisconnected();

    struct ClientState {
        quint16 blockSize{0};
        QByteArray buffer;
    };

private:
    qintptr _socketDescriptor;
    QTcpSocket *_socket{nullptr};
    ClientState _state;
    QByteArray _outBuffer;
    bool _disconnectEmitted{false};
};
