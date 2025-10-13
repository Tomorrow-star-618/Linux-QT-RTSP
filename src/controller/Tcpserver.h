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

// 设备ID枚举定义
enum DeviceID {
    DEVICE_SERVO = 1,    // 舵机设备
    DEVICE_CAMERA = 2,   // 摄像头设备
    DEVICE_LED = 3       // LED设备
};

// 操作ID枚举定义
enum OperationID {
    // 舵机操作ID (1-5)
    SERVO_UP = 1,      // 云台上
    SERVO_DOWN = 2,    // 云台下
    SERVO_LEFT = 3,    // 云台左
    SERVO_RIGHT = 4,   // 云台右
    SERVO_RESET = 5,   // 云台复位

    // 摄像头操作ID
    CAMERA_AI_ENABLE = 6,      // AI使能
    CAMERA_REGION_ENABLE = 7,  // 区域使能
    CAMERA_OBJECT_ENABLE = 8,  // 对象使能
    RTSP_ENABLE = 9            // RTSP使能
}; 

class TcpServerThread;

class Tcpserver : public QWidget {
    Q_OBJECT
public:
    explicit Tcpserver(QWidget* parent = nullptr);
    ~Tcpserver();

    // TCP传输函数 - 参数1: 目标摄像头ID（0表示广播）
    void Tcp_sent_info(int targetCameraId, int deviceId, int operationId, int operationValue);  // 发送设备操作信息
    void Tcp_sent_rect(int targetCameraId, int x, int y, int width, int height);    // 发送矩形框信息（绝对坐标）
    void Tcp_sent_rect(int targetCameraId, float x, float y, float width, float height); // 发送矩形框信息（归一化）
    void Tcp_sent_list(int targetCameraId, const QSet<int>& objectIds); // 发送目标ID列表
    
    // 重载版本 - 使用IP地址作为目标
    void Tcp_sent_info(const QString& targetIp, int deviceId, int operationId, int operationValue);
    void Tcp_sent_rect(const QString& targetIp, int x, int y, int width, int height);
    void Tcp_sent_rect(const QString& targetIp, float x, float y, float width, float height);
    void Tcp_sent_list(const QString& targetIp, const QSet<int>& objectIds);
    void startListen();                // 开始监听
    void stopListen();                 // 停止监听
    bool hasConnectedClients() const;  // 判断是否有已连接客户端
    
    // IP与摄像头ID映射管理
    void bindIpToCamera(const QString& ip, int cameraId);     // 绑定IP地址到摄像头ID
    void unbindIpFromCamera(const QString& ip);               // 解绑IP地址
    QString getIpForCamera(int cameraId) const;               // 获取摄像头对应的IP地址
    int getCameraForIp(const QString& ip) const;              // 获取IP对应的摄像头ID
    QMap<QString, int> getIpCameraMap() const;                // 获取IP到摄像头的映射表
    void setCurrentCameraId(int cameraId);                    // 设置当前选中的摄像头ID
    int getCurrentCameraId() const;                           // 获取当前选中的摄像头ID
    QStringList getConnectedIps() const;                      // 获取所有已连接的IP地址列表

    // 公有成员：文本显示区（为了让Controller能够访问）
    QTextBrowser* textBrowser;         // 文本显示区

signals:
    void tcpClientConnected(const QString& ip, quint16 port); // 新增：客户端连接成功信号
    void detectionDataReceived(const QString& detectionData); // 新增：检测数据接收信号

private slots:
    void clearTextBrowser();           // 清空文本显示
    void sendMessages();               // 发送消息给客户端
    void clientConnected();            // 有客户端连接
    void receiveMessages();            // 接收客户端消息
    void lockip();                     // 锁定/解锁IP输入框
    void socketStateChange(QAbstractSocket::SocketState state); // socket状态变化处理

private:
    void getLocalHostIP();             // 获取本地所有IP
    void processDetectionData(const QString& data); // 新增：处理检测数据
    QTcpServer* tcpServer;             // TCP服务器对象
    QList<QTcpSocket*> clientSockets;  // 已连接的客户端socket列表
    QMap<QString, QTcpSocket*> ipToSocketMap;  // IP地址到Socket的映射
    QMap<QString, int> ipToCameraMap;  // IP地址到摄像头ID的映射
    QMap<int, QString> cameraToIpMap;  // 摄像头ID到IP地址的映射
    int m_currentCameraId;             // 当前选中的摄像头ID
    QPushButton* pushButton[5];        // 按钮数组
    QLabel* label[2];                  // 标签数组
    QLineEdit* Ip_lineEdit;            // IP输入框
    QLineEdit* Sent_lineEdit;          // 发送消息输入框
    QComboBox* comboBox;               // 客户端端口选择下拉框
    QSpinBox* spinBox;                 // 监听端口号选择框
    QVBoxLayout* vBoxLayout;           // 垂直布局
    QHBoxLayout* hBoxLayout[4];        // 水平布局数组
    QWidget* hWidget[3];               // 水平布局用的widget
    QWidget* vWidget;                  // 主widget
    QList<QHostAddress> IPlist;        // 本地IP列表
    TcpServerThread* serverThread;     // 服务器线程指针

};

// TcpServerThread 线程类，用于处理服务器后台任务
class TcpServerThread : public QThread {
    Q_OBJECT
public:
    // 构造函数，接收Tcpserver指针和父对象指针
    TcpServerThread(Tcpserver* server, QObject* parent = nullptr);
    // 重写run方法，实现线程任务
    void run() override;
private:
    Tcpserver* m_server; // 保存Tcpserver对象指针
};
