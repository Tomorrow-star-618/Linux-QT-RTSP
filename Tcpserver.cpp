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
    pushButton[4] = new QPushButton("确定");

    pushButton[1]->setEnabled(false);

    label[0] = new QLabel("监听IP地址：");
    label[1] = new QLabel("监听端口：");

    comboBox = new QComboBox();
    comboBox->setFixedWidth(80); // 设置宽度为80
    comboBox->addItem("all"); // 初始添加all选项
    
    Ip_lineEdit = new QLineEdit("192.168.1.155");  
    Sent_lineEdit = new QLineEdit("TCPserver_sent_info");

    spinBox = new QSpinBox();
    spinBox->setRange(8890, 99999);
    spinBox->setValue(8890); // 设置默认端口为8890
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
    hBoxLayout[1]->addWidget(Ip_lineEdit);
    hBoxLayout[1]->addWidget(pushButton[4]);
    hBoxLayout[1]->addWidget(label[1]);
    hBoxLayout[1]->addWidget(spinBox);
    hWidget[1]->setLayout(hBoxLayout[1]);

    hBoxLayout[2]->addWidget(Sent_lineEdit);
    hBoxLayout[2]->addWidget(comboBox);
    hBoxLayout[2]->addWidget(pushButton[3]);
    hWidget[2]->setLayout(hBoxLayout[2]);

    vBoxLayout->addWidget(textBrowser);
    vBoxLayout->addWidget(hWidget[1]);
    vBoxLayout->addWidget(hWidget[0]);
    vBoxLayout->addWidget(hWidget[2]);
    vWidget->setLayout(vBoxLayout);
    setLayout(vBoxLayout);

    connect(pushButton[0], &QPushButton::clicked, this, &Tcpserver::startListen);
    connect(pushButton[1], &QPushButton::clicked, this, &Tcpserver::stopListen);
    connect(pushButton[2], &QPushButton::clicked, this, &Tcpserver::clearTextBrowser);
    connect(pushButton[3], &QPushButton::clicked, this, &Tcpserver::sendMessages);
    connect(pushButton[4], &QPushButton::clicked, this, &Tcpserver::lockip);
    connect(tcpServer, &QTcpServer::newConnection, this, &Tcpserver::clientConnected);
}

Tcpserver::~Tcpserver() {
    stopListen();
}

// 获取本地主机的所有IPv4地址，并将其添加到下拉框和IP列表中
void Tcpserver::getLocalHostIP()
{
    // 获取所有网络接口
    QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();
    // 遍历每个网络接口
    foreach (QNetworkInterface interface, list) {
        // 获取该接口的所有地址条目
        QList<QNetworkAddressEntry> entryList = interface.addressEntries();
        // 遍历每个地址条目
        foreach (QNetworkAddressEntry entry, entryList) {
            // 只处理IPv4地址
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                // 将IPv4地址添加到下拉框
                comboBox->addItem(entry.ip().toString());
                // 将IPv4地址添加到IP列表
                IPlist << entry.ip();
            }
        }
    }
}

void Tcpserver::startListen()
{
    //将字符串类型转化为QHostAddress类型，用于下一步的listen
    QString ipAddress = Ip_lineEdit->text();
    QHostAddress hostAddress(ipAddress);

    // 检查IP地址输入框内容是否有效（此处原代码判断条件有误，应该判断IP地址是否为空）
    if (!Ip_lineEdit->text().isEmpty()) {
        // 开始监听指定IP和端口
        tcpServer->listen(hostAddress, spinBox->value());
        // 设置“开始监听”按钮不可用
        pushButton[0]->setEnabled(false);
        // 设置“停止监听”按钮可用
        pushButton[1]->setEnabled(true);
        // 禁用端口号输入框
        spinBox->setEnabled(false);
        // 在文本浏览器中显示服务器IP地址
        textBrowser->append("服务器IP地址：" + Ip_lineEdit->text());
        // 在文本浏览器中显示正在监听的端口
        textBrowser->append("正在监听端口：" + spinBox->text());
        // 启动服务器线程（如果未启动）
        if (!serverThread) {
            serverThread = new TcpServerThread(this);
            serverThread->start();
        }
    }
}

void Tcpserver::stopListen()
{
    // 关闭TCP服务器
    tcpServer->close();

    // 断开并删除所有已连接的客户端socket
    foreach (QTcpSocket* sock, clientSockets) {
        if (sock->state() == QAbstractSocket::ConnectedState)
            sock->disconnectFromHost(); // 断开连接
        sock->deleteLater(); // 延迟删除socket对象
    }
    clientSockets.clear(); // 清空客户端socket列表

    // 更新按钮和控件状态
    pushButton[1]->setEnabled(false); // 停止监听按钮不可用
    pushButton[0]->setEnabled(true);  // 开始监听按钮可用
    comboBox->setEnabled(true);       // 端口选择下拉框可用
    spinBox->setEnabled(true);        // 端口号输入框可用

    // 在文本浏览器中显示已停止监听信息
    textBrowser->append("已停止监听端口：" + spinBox->text());

    // 关闭并销毁服务器线程
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

// 发送消息给所有已连接的客户端
void Tcpserver::sendMessages()
{
    // 获取发送框中的文本
    QString msg = Sent_lineEdit->text() + "\r\n"; // 每次发送信息添加换行符号\r\n
    QString selectedPort = comboBox->currentText();
    
    // 遍历所有客户端socket
    for (QTcpSocket* sock : clientSockets) {
        // 如果socket处于已连接状态
        if (sock->state() == QAbstractSocket::ConnectedState) {
            // 如果选择"all"或端口号匹配，则发送消息
            if (selectedPort == "all" || selectedPort == QString::number(sock->peerPort())) {
                sock->write(msg.toUtf8());
            }
        }
    }
    
    // 在文本浏览器中显示服务端发送的消息（显示内容也加\r\n）
    if (selectedPort == "all") {
        textBrowser->append("服务端[全部]：" + Sent_lineEdit->text() + "\r\n");
    } else {
        textBrowser->append(QString("服务端[端口%1]：%2\r\n").arg(selectedPort).arg(Sent_lineEdit->text()));
    }
}

void Tcpserver::clientConnected()
{
    // 获取下一个待处理的客户端连接
    QTcpSocket* clientSocket = tcpServer->nextPendingConnection();
    // 将新连接的客户端socket添加到clientSockets列表
    clientSockets << clientSocket;
    // 获取客户端IP地址
    QString ip = clientSocket->peerAddress().toString();
    // 获取客户端端口号
    quint16 port = clientSocket->peerPort();
    // 在文本浏览器中显示客户端已连接的信息
    textBrowser->append("客户端已连接");
    textBrowser->append("客户端ip地址:" + ip);
    textBrowser->append("客户端端口:" + QString::number(port));
    // 连接readyRead信号，用于接收客户端消息
    connect(clientSocket, &QTcpSocket::readyRead, this, &Tcpserver::receiveMessages);
    // 连接stateChanged信号，用于监控socket状态变化
    connect(clientSocket, &QTcpSocket::stateChanged, this, &Tcpserver::socketStateChange);

    // 新增：将端口号添加到comboBox（避免重复）
    QString portStr = QString::number(port);
    if (comboBox->findText(portStr) == -1) {
        comboBox->addItem(portStr);
    }
    // 发射客户端连接成功信号
    emit tcpClientConnected(ip, port);
}

void Tcpserver::receiveMessages()
{
    // 获取发送消息的客户端socket
    QTcpSocket* senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket) return; // 如果获取失败则直接返回
    // 获取客户端端口号
    quint16 port = senderSocket->peerPort();
    // 读取客户端发送的全部数据，并格式化显示
    QString messages = QString("客户端[%1]：%2").arg(port).arg(QString::fromUtf8(senderSocket->readAll()));
    // 在文本浏览器中显示客户端消息
    textBrowser->append(messages);
}

void Tcpserver::lockip()
{
    bool currentState = Ip_lineEdit->isReadOnly();
    Ip_lineEdit->setReadOnly(!currentState);

    if (Ip_lineEdit->isReadOnly()) {
        // 设置为只读状态，背景变灰
        Ip_lineEdit->setStyleSheet("background-color: lightgray;");
    } else {
        // 设置为可编辑状态，背景恢复白色
        Ip_lineEdit->setStyleSheet("background-color: white;");
    }
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

// TcpServerThread 构造函数，初始化线程并保存服务器指针
TcpServerThread::TcpServerThread(Tcpserver* server, QObject* parent)
    : QThread(parent), m_server(server) {}

void TcpServerThread::run()
{
    // 这里可以扩展为后台处理任务
    exec(); // 保持线程事件循环
} 

void Tcpserver::Tcp_sent_info(int deviceId, int operationId, int operationValue)
{
    // 构造固定格式的字符串：DEVICE_ID:OPERATION_ID:OPERATION_VALUE，并添加换行符
    QString message = QString("DEVICE_%1:OP_%2:VALUE_%3\r\n")
                     .arg(deviceId)
                     .arg(operationId)
                     .arg(operationValue);
    
    // 发送给所有连接的客户端
    for (QTcpSocket* sock : clientSockets) {
        if (sock->state() == QAbstractSocket::ConnectedState) {
            sock->write(message.toUtf8());
        }
    }
    
    // 在文本浏览器中显示发送的信息，并手动添加换行
    textBrowser->append("服务端发送设备信息：" + message);
//    textBrowser->append(QString("设备ID: %1, 操作ID: %2, 操作数值: %3\n")
//                       .arg(deviceId)
//                       .arg(operationId)
//                       .arg(operationValue));
}

void Tcpserver::Tcp_sent_rect(int x, int y, int width, int height)
{
    // 构造绝对坐标矩形框信息的字符串
    QString message = QString("RECT_ABS:%1:%2:%3:%4\r\n")
                         .arg(x)
                         .arg(y)
                         .arg(width)
                         .arg(height);
    // 发送给所有连接的客户端
    for (QTcpSocket* sock : clientSockets) {
        if (sock->state() == QAbstractSocket::ConnectedState) {
            sock->write(message.toUtf8());
        }
    }
    // 在文本浏览器中显示发送的矩形框信息
    textBrowser->append("服务端发送绝对矩形框信息：" + message);
//    textBrowser->append(QString("绝对矩形框: x=%1, y=%2, 尺寸: %3×%4\n")
//                       .arg(x)
//                       .arg(y)
//                       .arg(width)
//                       .arg(height));
}

void Tcpserver::Tcp_sent_rect(float x, float y, float width, float height)
{
    // 构造归一化矩形框信息的字符串，保留4位小数
    QString message = QString("RECT:%1:%2:%3:%4\r\n")
                         .arg(QString::number(x, 'f', 4))
                         .arg(QString::number(y, 'f', 4))
                         .arg(QString::number(width, 'f', 4))
                         .arg(QString::number(height, 'f', 4));
    // 发送给所有连接的客户端
    for (QTcpSocket* sock : clientSockets) {
        if (sock->state() == QAbstractSocket::ConnectedState) {
            sock->write(message.toUtf8());
        }
    }
    // 在文本浏览器中显示发送的矩形框信息
    textBrowser->append("服务端发送归一化矩形框信息：" + message);
//    textBrowser->append(QString("归一化矩形框: x=%1, y=%2, 尺寸: %3×%4\n")
//                       .arg(QString::number(x, 'f', 4))
//                       .arg(QString::number(y, 'f', 4))
//                       .arg(QString::number(width, 'f', 4))
//                       .arg(QString::number(height, 'f', 4)));
}

void Tcpserver::Tcp_sent_list(const QSet<int>& objectIds)
{
    // 构造对象列表信息的字符串：LIST:objectId1,objectId2,objectId3...，并添加换行符
    QStringList idList;
    for (int id : objectIds) {
        idList.append(QString::number(id));
    }
    
    QString message = QString("LIST:%1\r\n").arg(idList.join(","));
    
    // 发送给所有连接的客户端
    for (QTcpSocket* sock : clientSockets) {
        if (sock->state() == QAbstractSocket::ConnectedState) {
            sock->write(message.toUtf8());
        }
    }
    
    // 在文本浏览器中显示发送的对象列表信息
    textBrowser->append("服务端发送对象列表信息：" + message);
//    textBrowser->append(QString("选中对象数量: %1, 对象ID: %2\n")
//                       .arg(objectIds.size())
//                       .arg(idList.join(", ")));
} 

bool Tcpserver::hasConnectedClients() const
{
    for (QTcpSocket* sock : clientSockets) {
        if (sock && sock->state() == QAbstractSocket::ConnectedState) {
            return true;
        }
    }
    return false;
}
