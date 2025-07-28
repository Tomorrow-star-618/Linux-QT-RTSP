#include "Tcpserver.h"
#include <QHostAddress>
#include <QNetworkInterface>
#include <QMessageBox>
#include <QDebug>

Tcpserver::Tcpserver(QWidget* parent)
    : QWidget(parent), tcpServer(nullptr), serverThread(nullptr)
{
    this->setWindowTitle("Tcpserver");
    this->resize(800, 480);

    tcpServer = new QTcpServer(this);

    pushButton[0] = new QPushButton("开始监听");
    pushButton[1] = new QPushButton("停止监听");
    pushButton[2] = new QPushButton("清空文本");
    pushButton[3] = new QPushButton("发送消息");
    pushButton[1]->setEnabled(false);

    label[0] = new QLabel("监听IP地址：");
    label[1] = new QLabel("监听端口：");
    lineEdit = new QLineEdit("www.openedv.com正点原子论坛");
    comboBox = new QComboBox();
    spinBox = new QSpinBox();
    spinBox->setRange(10000, 99999);
    textBrowser = new QTextBrowser();

    hBoxLayout[0] = new QHBoxLayout();
    hBoxLayout[1] = new QHBoxLayout();
    hBoxLayout[2] = new QHBoxLayout();
    hBoxLayout[3] = new QHBoxLayout();
    hWidget[0] = new QWidget();
    hWidget[1] = new QWidget();
    hWidget[2] = new QWidget();
    vWidget = new QWidget();
    vBoxLayout = new QVBoxLayout();

    hBoxLayout[0]->addWidget(pushButton[0]);
    hBoxLayout[0]->addWidget(pushButton[1]);
    hBoxLayout[0]->addWidget(pushButton[2]);
    hWidget[0]->setLayout(hBoxLayout[0]);

    hBoxLayout[1]->addWidget(label[0]);
    hBoxLayout[1]->addWidget(comboBox);
    hBoxLayout[1]->addWidget(label[1]);
    hBoxLayout[1]->addWidget(spinBox);
    hWidget[1]->setLayout(hBoxLayout[1]);

    hBoxLayout[2]->addWidget(lineEdit);
    hBoxLayout[2]->addWidget(pushButton[3]);
    hWidget[2]->setLayout(hBoxLayout[2]);

    vBoxLayout->addWidget(textBrowser);
    vBoxLayout->addWidget(hWidget[1]);
    vBoxLayout->addWidget(hWidget[0]);
    vBoxLayout->addWidget(hWidget[2]);
    vWidget->setLayout(vBoxLayout);
    setLayout(vBoxLayout);

    getLocalHostIP();

    connect(pushButton[0], &QPushButton::clicked, this, &Tcpserver::startListen);
    connect(pushButton[1], &QPushButton::clicked, this, &Tcpserver::stopListen);
    connect(pushButton[2], &QPushButton::clicked, this, &Tcpserver::clearTextBrowser);
    connect(pushButton[3], &QPushButton::clicked, this, &Tcpserver::sendMessages);
    connect(tcpServer, &QTcpServer::newConnection, this, &Tcpserver::clientConnected);
}

Tcpserver::~Tcpserver() {
    stopListen();
}

void Tcpserver::getLocalHostIP()
{
    QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();
    foreach (QNetworkInterface interface, list) {
        QList<QNetworkAddressEntry> entryList = interface.addressEntries();
        foreach (QNetworkAddressEntry entry, entryList) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                comboBox->addItem(entry.ip().toString());
                IPlist << entry.ip();
            }
        }
    }
}

void Tcpserver::startListen()
{
    if (comboBox->currentIndex() != -1) {
        tcpServer->listen(IPlist[comboBox->currentIndex()], spinBox->value());
        pushButton[0]->setEnabled(false);
        pushButton[1]->setEnabled(true);
        comboBox->setEnabled(false);
        spinBox->setEnabled(false);
        textBrowser->append("服务器IP地址：" + comboBox->currentText());
        textBrowser->append("正在监听端口：" + spinBox->text());
        // 启动线程
        if (!serverThread) {
            serverThread = new TcpServerThread(this);
            serverThread->start();
        }
    }
}

void Tcpserver::stopListen()
{
    tcpServer->close();
    foreach (QTcpSocket* sock, clientSockets) {
        if (sock->state() == QAbstractSocket::ConnectedState)
            sock->disconnectFromHost();
        sock->deleteLater();
    }
    clientSockets.clear();
    pushButton[1]->setEnabled(false);
    pushButton[0]->setEnabled(true);
    comboBox->setEnabled(true);
    spinBox->setEnabled(true);
    textBrowser->append("已停止监听端口：" + spinBox->text());
    if (serverThread) {
        serverThread->quit();
        serverThread->wait();
        delete serverThread;
        serverThread = nullptr;
    }
}

void Tcpserver::clearTextBrowser()
{
    textBrowser->clear();
}

void Tcpserver::sendMessages()
{
    QString msg = lineEdit->text();
    for (QTcpSocket* sock : clientSockets) {
        if (sock->state() == QAbstractSocket::ConnectedState) {
            sock->write(msg.toUtf8());
        }
    }
    textBrowser->append("服务端：" + msg);
}

void Tcpserver::clientConnected()
{
    QTcpSocket* clientSocket = tcpServer->nextPendingConnection();
    clientSockets << clientSocket;
    QString ip = clientSocket->peerAddress().toString();
    quint16 port = clientSocket->peerPort();
    textBrowser->append("客户端已连接");
    textBrowser->append("客户端ip地址:" + ip);
    textBrowser->append("客户端端口:" + QString::number(port));
    connect(clientSocket, &QTcpSocket::readyRead, this, &Tcpserver::receiveMessages);
    connect(clientSocket, &QTcpSocket::stateChanged, this, &Tcpserver::socketStateChange);
}

void Tcpserver::receiveMessages()
{
    QTcpSocket* senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket) return;
    QString messages = "客户端：" + senderSocket->readAll();
    textBrowser->append(messages);
}

void Tcpserver::socketStateChange(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::UnconnectedState:
        textBrowser->append("scoket状态：UnconnectedState");
        break;
    case QAbstractSocket::ConnectedState:
        textBrowser->append("scoket状态：ConnectedState");
        break;
    case QAbstractSocket::ConnectingState:
        textBrowser->append("scoket状态：ConnectingState");
        break;
    case QAbstractSocket::HostLookupState:
        textBrowser->append("scoket状态：HostLookupState");
        break;
    case QAbstractSocket::ClosingState:
        textBrowser->append("scoket状态：ClosingState");
        break;
    case QAbstractSocket::ListeningState:
        textBrowser->append("scoket状态：ListeningState");
        break;
    case QAbstractSocket::BoundState:
        textBrowser->append("scoket状态：BoundState");
        break;
    default:
        break;
    }
}

TcpServerThread::TcpServerThread(Tcpserver* server, QObject* parent)
    : QThread(parent), m_server(server) {}

void TcpServerThread::run()
{
    // 这里可以扩展为后台处理任务
    exec(); // 保持线程事件循环
} 