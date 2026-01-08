#pragma once

#include <QtCore>
#include <QtNetwork/QTcpServer>
#include <QMutex>
#include <QThread>

class ClientHandler;

class Server : public QTcpServer
{
    Q_OBJECT
public:
    Server(quint16 port = 4000);
    ~Server();
    void shutdown();

private slots:
    void incomingConnection(qintptr socketHandler) override;
    void onClientDisconnected();

private:
    QMutex _clientsMutex;
    QSet<ClientHandler*> _activeHandlers;
    QSet<QThread*> _activeThreads;
};

