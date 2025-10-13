#include "Tcpserver.h"
#include <QHostAddress>
#include <QNetworkInterface>
#include <QMessageBox>
#include <QDebug>

Tcpserver::Tcpserver(QWidget* parent)
    : QWidget(parent), tcpServer(nullptr), serverThread(nullptr), m_currentCameraId(-1)
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
    
    Ip_lineEdit = new QLineEdit("192.168.1.156");  
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
    QString selectedIp = comboBox->currentText();
    
    // 遍历所有客户端socket
    for (QTcpSocket* sock : clientSockets) {
        // 如果socket处于已连接状态
        if (sock->state() == QAbstractSocket::ConnectedState) {
            QString ip = sock->peerAddress().toString();
            if (ip.startsWith("::ffff:")) {
                ip = ip.mid(7); // 去掉"::ffff:"前缀
            }
            
            // 如果选择"all"或IP地址匹配，则发送消息
            if (selectedIp == "all" || selectedIp == ip) {
                sock->write(msg.toUtf8());
            }
        }
    }
    
    // 在文本浏览器中显示服务端发送的消息
    if (selectedIp == "all") {
        textBrowser->append("服务端[全部]：" + Sent_lineEdit->text() + "\r\n");
    } else {
        // 显示IP和对应的摄像头ID（如果有）
        QString target;
        if (ipToCameraMap.contains(selectedIp)) {
            int cameraId = ipToCameraMap.value(selectedIp);
            target = QString("IP:%1|摄像头%2").arg(selectedIp).arg(cameraId);
        } else {
            target = QString("IP:%1|未绑定").arg(selectedIp);
        }
        textBrowser->append(QString("服务端[%1]：%2\r\n").arg(target).arg(Sent_lineEdit->text()));
    }
}

void Tcpserver::clientConnected()
{
    // 获取下一个待处理的客户端连接
    QTcpSocket* clientSocket = tcpServer->nextPendingConnection();
    // 将新连接的客户端socket添加到clientSockets列表
    clientSockets << clientSocket;
    // 获取客户端IP地址（去掉IPv6前缀，只保留IPv4）
    QString ip = clientSocket->peerAddress().toString();
    if (ip.startsWith("::ffff:")) {
        ip = ip.mid(7); // 去掉"::ffff:"前缀
    }
    // 获取客户端端口号
    quint16 port = clientSocket->peerPort();
    
    // 保存IP到Socket的映射
    ipToSocketMap.insert(ip, clientSocket);
    
    // 在文本浏览器中显示客户端已连接的信息
    textBrowser->append("========================================");
    textBrowser->append("✓ 新客户端连接");
    textBrowser->append("  IP地址: " + ip);
    textBrowser->append("  端口: " + QString::number(port));
    textBrowser->append("========================================");
    
    // 连接readyRead信号，用于接收客户端消息
    connect(clientSocket, &QTcpSocket::readyRead, this, &Tcpserver::receiveMessages);
    // 连接stateChanged信号，用于监控socket状态变化
    connect(clientSocket, &QTcpSocket::stateChanged, this, &Tcpserver::socketStateChange);
    // 连接disconnected信号，用于清理断开的连接
    connect(clientSocket, &QTcpSocket::disconnected, this, [this, ip]() {
        QTcpSocket* sender = qobject_cast<QTcpSocket*>(QObject::sender());
        
        textBrowser->append("========================================");
        textBrowser->append("✗ 客户端断开连接: " + ip);
        textBrowser->append("========================================");
        
        // 清理映射关系
        ipToSocketMap.remove(ip);
        if (ipToCameraMap.contains(ip)) {
            int cameraId = ipToCameraMap.value(ip);
            cameraToIpMap.remove(cameraId);
            ipToCameraMap.remove(ip);
            textBrowser->append(QString("  已解除IP[%1]与摄像头[%2]的绑定").arg(ip).arg(cameraId));
        }
        
        // 从comboBox移除该IP
        int index = comboBox->findText(ip);
        if (index > 0) { // 保留"all"选项
            comboBox->removeItem(index);
        }
        
        // 安全地移除socket
        if (sender) {
            clientSockets.removeOne(sender);
            sender->deleteLater();  // 延迟删除，避免立即释放导致问题
        }
    });

    // 新增：将IP地址添加到comboBox（避免重复）
    if (comboBox->findText(ip) == -1) {
        comboBox->addItem(ip);
    }
    
    // 发射客户端连接成功信号
    emit tcpClientConnected(ip, port);
}

void Tcpserver::receiveMessages()
{
    // 获取发送消息的客户端socket
    QTcpSocket* senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket) return; // 如果获取失败则直接返回
    
    // 获取客户端IP地址
    QString ip = senderSocket->peerAddress().toString();
    if (ip.startsWith("::ffff:")) {
        ip = ip.mid(7); // 去掉"::ffff:"前缀
    }
    
    // 读取客户端发送的全部数据
    QByteArray data = senderSocket->readAll();
    QString message = QString::fromUtf8(data);
    
    // 格式化显示消息，包含IP和对应的摄像头ID（如果有绑定）
    QString displayMessage;
    if (ipToCameraMap.contains(ip)) {
        int cameraId = ipToCameraMap.value(ip);
        displayMessage = QString("客户端[IP:%1|摄像头%2]：%3").arg(ip).arg(cameraId).arg(message);
    } else {
        displayMessage = QString("客户端[IP:%1|未绑定]：%2").arg(ip).arg(message);
    }
    textBrowser->append(displayMessage);
    
    // 检查是否为检测数据并进行处理（传递摄像头ID）
    int cameraId = ipToCameraMap.value(ip, -1); // 获取摄像头ID，未绑定则为-1
    processDetectionData(cameraId, message);
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

// ========== TCP传输函数（通过摄像头ID指定目标） ==========

void Tcpserver::Tcp_sent_info(int targetCameraId, int deviceId, int operationId, int operationValue)
{
    // 构造固定格式的字符串：DEVICE_ID:OPERATION_ID:OPERATION_VALUE，并添加换行符
    QString message = QString("DEVICE_%1:OP_%2:VALUE_%3\r\n")
                     .arg(deviceId)
                     .arg(operationId)
                     .arg(operationValue);
    
    // targetCameraId = 0 表示广播到所有客户端
    if (targetCameraId == 0) {
        int sentCount = 0;
        for (QTcpSocket* sock : clientSockets) {
            if (sock->state() == QAbstractSocket::ConnectedState) {
                sock->write(message.toUtf8());
                sock->flush();  // 立即发送数据
                sentCount++;
            }
        }
        textBrowser->append(QString("→ [广播到%1个客户端] %2")
                           .arg(sentCount)
                           .arg(message.trimmed()));
        return;
    }
    
    // 发送给指定摄像头ID对应的IP
    if (cameraToIpMap.contains(targetCameraId)) {
        QString targetIp = cameraToIpMap.value(targetCameraId);
        
        if (ipToSocketMap.contains(targetIp)) {
            QTcpSocket* sock = ipToSocketMap.value(targetIp);
            if (sock->state() == QAbstractSocket::ConnectedState) {
                sock->write(message.toUtf8());
                sock->flush();  // 立即发送数据
                textBrowser->append(QString("→ [摄像头%1|IP:%2] %3")
                                   .arg(targetCameraId)
                                   .arg(targetIp)
                                   .arg(message.trimmed()));
            } else {
                textBrowser->append(QString("⚠️ 摄像头%1对应的IP[%2]未连接")
                                   .arg(targetCameraId)
                                   .arg(targetIp));
            }
        } else {
            textBrowser->append(QString("⚠️ 未找到摄像头%1对应的IP[%2]的连接")
                               .arg(targetCameraId)
                               .arg(targetIp));
        }
    } else {
        textBrowser->append(QString("⚠️ 摄像头%1未绑定TCP客户端").arg(targetCameraId));
    }
}

void Tcpserver::Tcp_sent_rect(int targetCameraId, int x, int y, int width, int height)
{
    // 构造绝对坐标矩形框信息的字符串
    QString message = QString("RECT_ABS:%1:%2:%3:%4\r\n")
                         .arg(x)
                         .arg(y)
                         .arg(width)
                         .arg(height);
    
    // targetCameraId = 0 表示广播到所有客户端
    if (targetCameraId == 0) {
        for (QTcpSocket* sock : clientSockets) {
            if (sock->state() == QAbstractSocket::ConnectedState) {
                sock->write(message.toUtf8());
                sock->flush();  // 立即发送数据
            }
        }
        textBrowser->append("→ [广播] " + message.trimmed());
        return;
    }
    
    // 发送给指定摄像头ID对应的IP
    if (cameraToIpMap.contains(targetCameraId)) {
        QString targetIp = cameraToIpMap.value(targetCameraId);
        
        if (ipToSocketMap.contains(targetIp)) {
            QTcpSocket* sock = ipToSocketMap.value(targetIp);
            if (sock->state() == QAbstractSocket::ConnectedState) {
                sock->write(message.toUtf8());
                sock->flush();  // 立即发送数据
                textBrowser->append(QString("→ [摄像头%1|IP:%2] %3")
                                   .arg(targetCameraId)
                                   .arg(targetIp)
                                   .arg(message.trimmed()));
            }
        }
    } else {
        textBrowser->append(QString("⚠️ 摄像头%1未绑定TCP客户端").arg(targetCameraId));
    }
}

void Tcpserver::Tcp_sent_rect(int targetCameraId, float x, float y, float width, float height)
{
    // 构造归一化矩形框信息的字符串，保留4位小数
    QString message = QString("RECT:%1:%2:%3:%4\r\n")
                         .arg(QString::number(x, 'f', 4))
                         .arg(QString::number(y, 'f', 4))
                         .arg(QString::number(width, 'f', 4))
                         .arg(QString::number(height, 'f', 4));
    
    // targetCameraId = 0 表示广播到所有客户端
    if (targetCameraId == 0) {
        for (QTcpSocket* sock : clientSockets) {
            if (sock->state() == QAbstractSocket::ConnectedState) {
                sock->write(message.toUtf8());
                sock->flush();  // 立即发送数据
            }
        }
        textBrowser->append("→ [广播] " + message.trimmed());
        return;
    }
    
    // 发送给指定摄像头ID对应的IP
    if (cameraToIpMap.contains(targetCameraId)) {
        QString targetIp = cameraToIpMap.value(targetCameraId);
        
        if (ipToSocketMap.contains(targetIp)) {
            QTcpSocket* sock = ipToSocketMap.value(targetIp);
            if (sock->state() == QAbstractSocket::ConnectedState) {
                sock->write(message.toUtf8());
                sock->flush();  // 立即发送数据
                textBrowser->append(QString("→ [摄像头%1|IP:%2] %3")
                                   .arg(targetCameraId)
                                   .arg(targetIp)
                                   .arg(message.trimmed()));
            }
        }
    } else {
        textBrowser->append(QString("⚠️ 摄像头%1未绑定TCP客户端").arg(targetCameraId));
    }
}

void Tcpserver::Tcp_sent_list(int targetCameraId, const QSet<int>& objectIds)
{
    // 构造对象列表信息的字符串：LIST:objectId1,objectId2,objectId3...，并添加换行符
    QStringList idList;
    for (int id : objectIds) {
        idList.append(QString::number(id));
    }
    
    QString message = QString("LIST:%1\r\n").arg(idList.join(","));
    
    // targetCameraId = 0 表示广播到所有客户端
    if (targetCameraId == 0) {
        for (QTcpSocket* sock : clientSockets) {
            if (sock->state() == QAbstractSocket::ConnectedState) {
                sock->write(message.toUtf8());
                sock->flush();  // 立即发送数据
            }
        }
        textBrowser->append("→ [广播] " + message.trimmed());
        return;
    }
    
    // 发送给指定摄像头ID对应的IP
    if (cameraToIpMap.contains(targetCameraId)) {
        QString targetIp = cameraToIpMap.value(targetCameraId);
        
        if (ipToSocketMap.contains(targetIp)) {
            QTcpSocket* sock = ipToSocketMap.value(targetIp);
            if (sock->state() == QAbstractSocket::ConnectedState) {
                sock->write(message.toUtf8());
                sock->flush();  // 立即发送数据
                textBrowser->append(QString("→ [摄像头%1|IP:%2] %3")
                                   .arg(targetCameraId)
                                   .arg(targetIp)
                                   .arg(message.trimmed()));
            }
        }
    } else {
        textBrowser->append(QString("⚠️ 摄像头%1未绑定TCP客户端").arg(targetCameraId));
    }
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

// 处理检测数据的函数，当接收到DETECTIONS格式的数据时触发图像保存
// 参数：cameraId - 发送数据的摄像头ID，-1表示未绑定
void Tcpserver::processDetectionData(int cameraId, const QString& data)
{
    // 去除首尾空白字符并检查数据是否以DETECTIONS开头
    QString trimmedData = data.trimmed();
    if (!trimmedData.startsWith("DETECTIONS")) {
        return; // 不是检测数据格式，直接返回
    }
    
    // 解析检测数据格式：DETECTIONS:6|0:person:209:2:506:475:0.843|62:tv:633:313:57:62:0.774|...
    QStringList parts = trimmedData.split(":");
    if (parts.size() < 2) {
        // 数据格式不正确，记录错误信息
        textBrowser->append("⚠️ 检测数据格式错误：" + trimmedData);
        return;
    }
    
    // 提取检测对象数量和详细信息
    QString detectionInfo = parts[1]; // 获取"6|0:person:209:2:506:475:0.843|62:tv:..."部分
    QStringList objectParts = detectionInfo.split("|");
    
    if (objectParts.isEmpty()) {
        textBrowser->append("⚠️ 检测数据为空");
        return;
    }
    
    // 第一个部分是对象总数
    int totalObjects = objectParts[0].toInt();
    
    // 解析每个检测对象的信息
    QStringList categories;
    int objectIndex = 1;
    
    for (int i = 1; i < objectParts.size(); ++i) {
        QString objectInfo = objectParts[i];
        QStringList objectDetails = objectInfo.split(":");
        
        // 格式：class_id:class_name:x:y:width:height:confidence
        if (objectDetails.size() >= 2) {
            QString className = objectDetails[1]; // 获取类别名称
            categories.append(QString("%1:%2").arg(objectIndex).arg(className));
            objectIndex++;
        }
    }
    
    // 构建处理后的数据格式
    QString processedData;
    if (!categories.isEmpty()) {
        processedData = QString("%1个物体,%2").arg(totalObjects).arg(categories.join(";"));
    } else {
        processedData = QString("%1个物体").arg(totalObjects);
    }
    
    
    // 发射信号给controller，传递摄像头ID和处理后的数据
    emit detectionDataReceived(cameraId, processedData);
}

// ========== IP与摄像头ID映射管理函数 ==========

// 绑定IP地址到摄像头ID
void Tcpserver::bindIpToCamera(const QString& ip, int cameraId)
{
    // 如果该摄像头ID已经绑定了其他IP，先解绑
    if (cameraToIpMap.contains(cameraId)) {
        QString oldIp = cameraToIpMap.value(cameraId);
        ipToCameraMap.remove(oldIp);
        textBrowser->append(QString("⚠️ 摄像头%1已从IP[%2]解绑").arg(cameraId).arg(oldIp));
    }
    
    // 如果该IP已经绑定了其他摄像头，先解绑
    if (ipToCameraMap.contains(ip)) {
        int oldCameraId = ipToCameraMap.value(ip);
        cameraToIpMap.remove(oldCameraId);
        textBrowser->append(QString("⚠️ IP[%1]已从摄像头%2解绑").arg(ip).arg(oldCameraId));
    }
    
    // 建立新的绑定关系
    ipToCameraMap.insert(ip, cameraId);
    cameraToIpMap.insert(cameraId, ip);
    
    textBrowser->append("========================================");
    textBrowser->append(QString("✓ 摄像头%1已成功连接到IP地址%2").arg(cameraId).arg(ip));
    textBrowser->append("========================================");
}

// 解绑IP地址
void Tcpserver::unbindIpFromCamera(const QString& ip)
{
    if (ipToCameraMap.contains(ip)) {
        int cameraId = ipToCameraMap.value(ip);
        ipToCameraMap.remove(ip);
        cameraToIpMap.remove(cameraId);
        
        textBrowser->append("========================================");
        textBrowser->append(QString("✓ 已解绑: IP[%1] ⇔ 摄像头%2").arg(ip).arg(cameraId));
        textBrowser->append("========================================");
    }
}

// 获取摄像头对应的IP地址
QString Tcpserver::getIpForCamera(int cameraId) const
{
    return cameraToIpMap.value(cameraId, QString());
}

// 获取IP对应的摄像头ID
int Tcpserver::getCameraForIp(const QString& ip) const
{
    return ipToCameraMap.value(ip, -1);
}

// 获取IP到摄像头的映射表
QMap<QString, int> Tcpserver::getIpCameraMap() const
{
    return ipToCameraMap;
}

// 设置当前选中的摄像头ID
void Tcpserver::setCurrentCameraId(int cameraId)
{
    m_currentCameraId = cameraId;
    
    // 如果该摄像头有绑定的IP，更新comboBox选中项
    if (cameraToIpMap.contains(cameraId)) {
        QString ip = cameraToIpMap.value(cameraId);
        int index = comboBox->findText(ip);
        if (index != -1) {
            comboBox->blockSignals(true);
            comboBox->setCurrentIndex(index);
            comboBox->blockSignals(false);
        }
    }
}

// 获取当前选中的摄像头ID
int Tcpserver::getCurrentCameraId() const
{
    return m_currentCameraId;
}

// 获取所有已连接的IP地址列表
QStringList Tcpserver::getConnectedIps() const
{
    return ipToSocketMap.keys();
}

// ========== TCP传输函数（通过IP地址指定目标） ==========

void Tcpserver::Tcp_sent_info(const QString& targetIp, int deviceId, int operationId, int operationValue)
{
    QString message = QString("DEVICE_%1:OP_%2:VALUE_%3\r\n")
                     .arg(deviceId)
                     .arg(operationId)
                     .arg(operationValue);
    
    if (targetIp.isEmpty() || targetIp == "all") {
        // 广播到所有客户端
        int sentCount = 0;
        for (QTcpSocket* sock : clientSockets) {
            if (sock->state() == QAbstractSocket::ConnectedState) {
                sock->write(message.toUtf8());
                sock->flush();  // 立即发送数据
                sentCount++;
            }
        }
        textBrowser->append(QString("→ [广播到%1个客户端] %2")
                           .arg(sentCount)
                           .arg(message.trimmed()));
    } else if (ipToSocketMap.contains(targetIp)) {
        QTcpSocket* sock = ipToSocketMap.value(targetIp);
        if (sock->state() == QAbstractSocket::ConnectedState) {
            sock->write(message.toUtf8());
                sock->flush();  // 立即发送数据
            int cameraId = ipToCameraMap.value(targetIp, -1);
            if (cameraId > 0) {
                textBrowser->append(QString("→ [IP:%1|摄像头%2] %3")
                                   .arg(targetIp)
                                   .arg(cameraId)
                                   .arg(message.trimmed()));
            } else {
                textBrowser->append(QString("→ [IP:%1|未绑定] %2")
                                   .arg(targetIp)
                                   .arg(message.trimmed()));
            }
        }
    } else {
        textBrowser->append(QString("⚠️ IP[%1]未连接").arg(targetIp));
    }
}

void Tcpserver::Tcp_sent_rect(const QString& targetIp, int x, int y, int width, int height)
{
    QString message = QString("RECT_ABS:%1:%2:%3:%4\r\n")
                         .arg(x)
                         .arg(y)
                         .arg(width)
                         .arg(height);
    
    if (targetIp.isEmpty() || targetIp == "all") {
        for (QTcpSocket* sock : clientSockets) {
            if (sock->state() == QAbstractSocket::ConnectedState) {
                sock->write(message.toUtf8());
                sock->flush();  // 立即发送数据
            }
        }
        textBrowser->append("→ [广播] " + message.trimmed());
    } else if (ipToSocketMap.contains(targetIp)) {
        QTcpSocket* sock = ipToSocketMap.value(targetIp);
        if (sock->state() == QAbstractSocket::ConnectedState) {
            sock->write(message.toUtf8());
                sock->flush();  // 立即发送数据
            int cameraId = ipToCameraMap.value(targetIp, -1);
            if (cameraId > 0) {
                textBrowser->append(QString("→ [IP:%1|摄像头%2] %3")
                                   .arg(targetIp)
                                   .arg(cameraId)
                                   .arg(message.trimmed()));
            } else {
                textBrowser->append(QString("→ [IP:%1] %2")
                                   .arg(targetIp)
                                   .arg(message.trimmed()));
            }
        }
    } else {
        textBrowser->append(QString("⚠️ IP[%1]未连接").arg(targetIp));
    }
}

void Tcpserver::Tcp_sent_rect(const QString& targetIp, float x, float y, float width, float height)
{
    QString message = QString("RECT:%1:%2:%3:%4\r\n")
                         .arg(QString::number(x, 'f', 4))
                         .arg(QString::number(y, 'f', 4))
                         .arg(QString::number(width, 'f', 4))
                         .arg(QString::number(height, 'f', 4));
    
    if (targetIp.isEmpty() || targetIp == "all") {
        for (QTcpSocket* sock : clientSockets) {
            if (sock->state() == QAbstractSocket::ConnectedState) {
                sock->write(message.toUtf8());
                sock->flush();  // 立即发送数据
            }
        }
        textBrowser->append("→ [广播] " + message.trimmed());
    } else if (ipToSocketMap.contains(targetIp)) {
        QTcpSocket* sock = ipToSocketMap.value(targetIp);
        if (sock->state() == QAbstractSocket::ConnectedState) {
            sock->write(message.toUtf8());
                sock->flush();  // 立即发送数据
            int cameraId = ipToCameraMap.value(targetIp, -1);
            if (cameraId > 0) {
                textBrowser->append(QString("→ [IP:%1|摄像头%2] %3")
                                   .arg(targetIp)
                                   .arg(cameraId)
                                   .arg(message.trimmed()));
            } else {
                textBrowser->append(QString("→ [IP:%1] %2")
                                   .arg(targetIp)
                                   .arg(message.trimmed()));
            }
        }
    } else {
        textBrowser->append(QString("⚠️ IP[%1]未连接").arg(targetIp));
    }
}

void Tcpserver::Tcp_sent_list(const QString& targetIp, const QSet<int>& objectIds)
{
    QStringList idList;
    for (int id : objectIds) {
        idList.append(QString::number(id));
    }
    
    QString message = QString("LIST:%1\r\n").arg(idList.join(","));
    
    if (targetIp.isEmpty() || targetIp == "all") {
        for (QTcpSocket* sock : clientSockets) {
            if (sock->state() == QAbstractSocket::ConnectedState) {
                sock->write(message.toUtf8());
                sock->flush();  // 立即发送数据
            }
        }
        textBrowser->append("→ [广播] " + message.trimmed());
    } else if (ipToSocketMap.contains(targetIp)) {
        QTcpSocket* sock = ipToSocketMap.value(targetIp);
        if (sock->state() == QAbstractSocket::ConnectedState) {
            sock->write(message.toUtf8());
                sock->flush();  // 立即发送数据
            int cameraId = ipToCameraMap.value(targetIp, -1);
            if (cameraId > 0) {
                textBrowser->append(QString("→ [IP:%1|摄像头%2] %3")
                                   .arg(targetIp)
                                   .arg(cameraId)
                                   .arg(message.trimmed()));
            } else {
                textBrowser->append(QString("→ [IP:%1] %2")
                                   .arg(targetIp)
                                   .arg(message.trimmed()));
            }
        }
    } else {
        textBrowser->append(QString("⚠️ IP[%1]未连接").arg(targetIp));
    }
}
