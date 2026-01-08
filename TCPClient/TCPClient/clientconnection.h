#pragma once

#include <QObject>
#include <QTcpSocket>

class ClientConnection : public QObject
{
    Q_OBJECT

public:
    explicit ClientConnection(QObject *parent = nullptr);
    ~ClientConnection();

    void setServer(const QString &ip, quint16 port);
    void connectToHost();
    void sendPayload(const QString &payload);
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void messageReceived(const QString &message);
    void errorOccurred(const QString &errorString);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket *_socket;
    quint16 _blockSize;
    QString _serverIp;
    quint16 _serverPort;
};
