#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QHostAddress>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QDataStream>
#include <QRegularExpression>
#include <QAbstractSocket>

const QString SERVER_IP = "127.0.0.1";
constexpr quint16 PORT = 4000;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _client(new ClientConnection(this))
    , _serverIp(SERVER_IP)
    , _serverPort(PORT)
{
    ui->setupUi(this);
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::onConnect);
    connect(ui->submitButton, &QPushButton::clicked, this, &MainWindow::onSendToServer);
    connect(_client, &ClientConnection::connected, this, &MainWindow::onConnected);
    connect(_client, &ClientConnection::disconnected, this, &MainWindow::onDisconnected);
    connect(_client, &ClientConnection::errorOccurred, this, &MainWindow::onSocketError);
    connect(_client, &ClientConnection::messageReceived, this, &MainWindow::onMessageReceived);

    // Settings menu for configuring server IP/port
    QMenu *settingsMenu = menuBar()->addMenu(tr("Settings"));
    QAction *configureAction = settingsMenu->addAction(tr("Configure Server..."));
    connect(configureAction, &QAction::triggered, this, &MainWindow::onConfigureServer);

    statusBar()->showMessage(tr("Server: %1:%2").arg(_serverIp).arg(_serverPort));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onConnected()
{
    statusBar()->showMessage(tr("Connected to %1:%2").arg(_serverIp).arg(_serverPort));
    QMessageBox::information(this, tr("Connected"), tr("Successfully connected to server."));
}

void MainWindow::onDisconnected()
{
    statusBar()->showMessage(tr("Disconnected"), 3000);
}

void MainWindow::onSocketError(const QString &socketError)
{
    statusBar()->showMessage(tr("Socket error: %1").arg(socketError), 5000);
    QMessageBox::critical(this, tr("Socket Error"), socketError);
}

void MainWindow::onConnect()
{
    if (!_client)
        _client = new ClientConnection(this);

    if (_client->isConnected())
    {
        QMessageBox::information(this, tr("Already Connected"), tr("Already connected to server."));
        return;
    }

    _client->setServer(_serverIp, _serverPort);
    statusBar()->showMessage(tr("Connecting to %1:%2...").arg(_serverIp).arg(_serverPort));
    _client->connectToHost();
}

void MainWindow::onSendToServer()
{
    const QString userId = ui->userIdLineEdit->text();
    const QString userName = ui->userNameLineEdit->text();
    const QString email = ui->emailLineEdit->text();

    // Validate user ID: must be integer number
    bool isValidId = false;
    userId.toUInt(&isValidId);
    if (!isValidId || userId.isEmpty())
    {
        QMessageBox::warning(this, tr("Invalid User ID"), tr("User ID must be a number."));
        return;
    }

    // Validate email using regular expression
    QRegularExpression emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    if (!emailRegex.match(email).hasMatch())
    {
        QMessageBox::warning(this, tr("Invalid Email"), tr("Email address must be in format: username@domain.com"));
        return;
    }

    // Check if connected to server
    if (!_client)
    {
        QMessageBox::warning(this, tr("Not Connected"), tr("Please connect to server first."));
        return;
    }

    QString payload = "UserID:" + userId + " userName:" + userName + " email:" + email;
    _client->sendPayload(payload);

    qDebug() << "Submitted:" << userId << userName << email;
    statusBar()->showMessage(tr("Submitted: %1, %2, %3").arg(userId, userName, email), 5000);
}

void MainWindow::onConfigureServer()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Configuration"));
    dlg.setWindowModality(Qt::WindowModal);
    dlg.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dlg.setWindowState(dlg.windowState() & ~Qt::WindowFullScreen);

    QFormLayout *form = new QFormLayout(&dlg);
    QLineEdit *ipEdit = new QLineEdit(_serverIp, &dlg);
    QSpinBox *portSpin = new QSpinBox(&dlg);
    portSpin->setRange(1, 65535);
    portSpin->setValue(_serverPort);

    form->addRow(tr("Server IP:"), ipEdit);
    form->addRow(tr("Port:"), portSpin);

    QDialogButtonBox *button = new QDialogButtonBox(QDialogButtonBox::Ok, &dlg);
    form->addRow(button);
    connect(button, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    dlg.setFixedSize(dlg.sizeHint());

    if (dlg.exec() != QDialog::Accepted)
        return;

    QString ip = ipEdit->text();
    QHostAddress addr;
    if (!addr.setAddress(ip)) {
        QMessageBox::warning(this, tr("Invalid IP"), tr("The IP address entered is invalid."));
        return;
    }

    _serverIp = ip;
    _serverPort = static_cast<quint16>(portSpin->value());

    statusBar()->showMessage(tr("Server: %1:%2").arg(_serverIp).arg(_serverPort));
}

void MainWindow::onMessageReceived(const QString &message)
{
    statusBar()->showMessage(message, 5000);
}
