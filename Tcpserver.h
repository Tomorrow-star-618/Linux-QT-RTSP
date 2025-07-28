#pragma once
#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QList>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>

class TcpServerThread;

class Tcpserver : public QWidget {
    Q_OBJECT
public:
    explicit Tcpserver(QWidget* parent = nullptr);
    ~Tcpserver();

private slots:
    void startListen();
    void stopListen();
    void clearTextBrowser();
    void sendMessages();
    void clientConnected();
    void receiveMessages();
    void socketStateChange(QAbstractSocket::SocketState state);

private:
    void getLocalHostIP();
    QTcpServer* tcpServer;
    QList<QTcpSocket*> clientSockets;
    QPushButton* pushButton[4];
    QLabel* label[2];
    QLineEdit* lineEdit;
    QComboBox* comboBox;
    QSpinBox* spinBox;
    QTextBrowser* textBrowser;
    QVBoxLayout* vBoxLayout;
    QHBoxLayout* hBoxLayout[4];
    QWidget* hWidget[3];
    QWidget* vWidget;
    QList<QHostAddress> IPlist;
    TcpServerThread* serverThread;
};

class TcpServerThread : public QThread {
    Q_OBJECT
public:
    TcpServerThread(Tcpserver* server, QObject* parent = nullptr);
    void run() override;
private:
    Tcpserver* m_server;
}; 