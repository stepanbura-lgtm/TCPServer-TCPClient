#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <csignal>

#include "server.h"

static void signalHandler(int)
{
    QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Parse command-line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("TCP Server - Listen for client connections and echo messages back.");
    parser.addHelpOption();
    
    QCommandLineOption portOption(
        QStringList() << "p" << "port",
        "Port number to listen on (default: 4000).",
        "port",
        "4000"
    );
    parser.addOption(portOption);
    
    parser.process(a);
    
    bool ok = false;
    quint16 port = parser.value(portOption).toUShort(&ok);
    
    if (!ok || port == 0)
    {
        qCritical() << "Error: Invalid port number. Must be a number between 1 and 65535.";
        return 1;
    }
    
    Server s(port);

    QObject::connect(&a, &QCoreApplication::aboutToQuit, &s, &Server::shutdown);

    // Handle Ctrl+C and SIGTERM
    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    return a.exec();
}
