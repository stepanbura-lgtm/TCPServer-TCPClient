#include "server.h"
#include "clienthandler.h"
#include <QMutexLocker>

Server::Server(quint16 port)
{
    if (!this->listen(QHostAddress::Any, port))
    {
        qCritical() << "Unable to listen on port" << port;
    }
    qInfo() << "Server listening on port" << port;
}

Server::~Server()
{
    QMutexLocker lock(&_clientsMutex);
    
    // Stop and join all threads synchronously, then delete handlers and threads.
    for (QThread *thread : _activeThreads)
    {
        if (!thread)
            continue;
        // Ask thread to quit and wait for it to finish
        thread->quit();
        thread->wait(500);
    }

    // Delete handlers and threads now that threads have finished
    for (ClientHandler *handler : _activeHandlers)
    {
        if (handler)
            handler->deleteLater();
    }

    for (QThread *thread : _activeThreads)
    {
        if (thread)
            thread->deleteLater();
    }

    _activeThreads.clear();
    _activeHandlers.clear();
}

void Server::shutdown()
{
    qInfo() << "Shutting down server...";

    if (this->isListening())
        this->close();
}

void Server::incomingConnection(qintptr socketHandler)
{
    // Create handler and dedicated thread for this client
    ClientHandler *handler = new ClientHandler(socketHandler, nullptr);
    QThread *thread = new QThread(this);

    {
        QMutexLocker lock(&_clientsMutex);
        _activeHandlers.insert(handler);
        _activeThreads.insert(thread);
    }

    // Move handler to worker thread
    handler->moveToThread(thread);

    // Connect signals
    connect(handler, &ClientHandler::clientDisconnected, this, &Server::onClientDisconnected);
    connect(thread, &QThread::started, handler, &ClientHandler::start);
    connect(handler, &ClientHandler::clientDisconnected, thread, &QThread::quit);

    thread->start();
}

void Server::onClientDisconnected()
{
    ClientHandler *handler = qobject_cast<ClientHandler*>(sender());
    if (!handler)
        return;
    qInfo() << "Client disconnected (handler)" << handler;
}
