#include <QMainWindow>
#include "clientconnection.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnect();
    void onSendToServer();
    void onConfigureServer();
    void onConnected();
    void onDisconnected();
    void onSocketError(const QString &socketError);
    void onMessageReceived(const QString &message);

private:
    Ui::MainWindow *ui;
    ClientConnection *_client;
    QByteArray _data;
    QString _serverIp;
    quint16 _serverPort;
};
