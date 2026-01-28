#pragma once

#include <QDateTime>
#include <QHostAddress>
#include <QMap>
#include <QObject>
#include <QTimer>
#include <QUdpSocket>

/**
 * @brief 设备信息结构体
 * 用于存储通过UDP广播发现的设备信息
 */
struct DiscoveredDevice {
  QString deviceId;        // 设备唯一标识符
  QString deviceName;      // 设备名称
  QString rtspUrl;         // RTSP流地址
  QString ipAddress;       // 设备IP地址
  quint16 rtspPort;        // RTSP端口号（默认554）
  QString manufacturer;    // 设备制造商
  QString model;           // 设备型号
  QString firmwareVersion; // 固件版本
  QDateTime lastSeen;      // 最后一次发现时间
  bool isOnline;           // 设备是否在线

  // 默认构造函数
  DiscoveredDevice() : rtspPort(554), isOnline(true) {}

  // 检查设备是否仍然活跃（30秒内有响应则认为活跃）
  bool isActive() const {
    return lastSeen.secsTo(QDateTime::currentDateTime()) < 30;
  }
};

/**
 * @brief UDP广播协同模块 - 设备发现类
 *
 * 功能：
 * 1. 监听UDP广播端口，接收设备的广播消息
 * 2. 发送设备发现请求广播
 * 3. 维护已发现设备列表
 * 4. 支持零配置接入（自动获取RTSP地址）
 *
 * 协议格式（JSON）:
 * {
 *     "type": "discovery_response" | "discovery_request" | "heartbeat",
 *     "device_id": "unique_device_id",
 *     "device_name": "Camera Name",
 *     "rtsp_url": "rtsp://192.168.1.100/live/0",
 *     "rtsp_port": 554,
 *     "ip_address": "192.168.1.100",
 *     "manufacturer": "Manufacturer Name",
 *     "model": "Model Name",
 *     "firmware_version": "1.0.0"
 * }
 */
class DeviceDiscovery : public QObject {
  Q_OBJECT

public:
  // 默认UDP广播端口
  static const quint16 DEFAULT_DISCOVERY_PORT = 8888;

  explicit DeviceDiscovery(QObject *parent = nullptr);
  ~DeviceDiscovery();

  // ========== 核心功能 ==========

  /**
   * @brief 启动设备发现服务
   * @param port UDP监听端口号，默认8888
   * @return 是否启动成功
   */
  bool startDiscovery(quint16 port = DEFAULT_DISCOVERY_PORT);

  /**
   * @brief 停止设备发现服务
   */
  void stopDiscovery();

  /**
   * @brief 发送设备发现广播请求
   * 向局域网广播发现请求，所有支持的设备应当响应
   */
  void sendDiscoveryRequest();

  /**
   * @brief 检查设备发现服务是否正在运行
   * @return 是否运行中
   */
  bool isRunning() const { return m_running; }

  /**
   * @brief 获取当前发现的所有设备列表
   * @return 设备列表
   */
  QList<DiscoveredDevice> getDiscoveredDevices() const;

  /**
   * @brief 根据设备ID获取设备信息
   * @param deviceId 设备唯一标识符
   * @return 设备信息，如果不存在则返回空设备
   */
  DiscoveredDevice getDevice(const QString &deviceId) const;

  /**
   * @brief 清除所有已发现的设备
   */
  void clearDevices();

  /**
   * @brief 设置设备超时时间（秒）
   * 超过此时间未收到心跳的设备将被标记为离线
   * @param seconds 超时秒数，默认30秒
   */
  void setDeviceTimeout(int seconds) { m_deviceTimeout = seconds; }

  /**
   * @brief 设置自动发送发现请求的间隔（毫秒）
   * @param msec 间隔毫秒数，0表示禁用自动发送
   */
  void setAutoDiscoveryInterval(int msec);

  /**
   * @brief 向指定设备发送连接请求
   * 设备收到此请求后，应主动建立TCP连接到上位机
   * @param device 目标设备
   * @param tcpPort 上位机TCP监听端口
   * @return 是否发送成功
   */
  bool sendConnectionRequest(const DiscoveredDevice &device, quint16 tcpPort);

  /**
   * @brief 向指定IP发送连接请求
   * @param deviceIp 目标设备IP
   * @param tcpPort 上位机TCP监听端口
   * @return 是否发送成功
   */
  bool sendConnectionRequest(const QString &deviceIp, quint16 tcpPort);

signals:
  /**
   * @brief 发现新设备信号
   * @param device 新发现的设备信息
   */
  void deviceDiscovered(const DiscoveredDevice &device);

  /**
   * @brief 设备更新信号（设备信息变化或心跳更新）
   * @param device 更新后的设备信息
   */
  void deviceUpdated(const DiscoveredDevice &device);

  /**
   * @brief 设备离线信号
   * @param deviceId 离线设备的ID
   */
  void deviceOffline(const QString &deviceId);

  /**
   * @brief 错误信号
   * @param errorMessage 错误描述
   */
  void errorOccurred(const QString &errorMessage);

private slots:
  /**
   * @brief 处理接收到的UDP数据报
   */
  void onReadyRead();

  /**
   * @brief 定时检查设备在线状态
   */
  void onCheckDevicesTimeout();

  /**
   * @brief 自动发送发现请求
   */
  void onAutoDiscoveryTimeout();

private:
  /**
   * @brief 解析UDP数据报
   * @param data 接收到的数据
   * @param sender 发送方地址
   * @param senderPort 发送方端口
   */
  void parseMessage(const QByteArray &data, const QHostAddress &sender,
                    quint16 senderPort);

  /**
   * @brief 处理设备发现响应
   * @param jsonData JSON数据
   * @param sender 发送方地址
   */
  void handleDiscoveryResponse(const QJsonObject &jsonData,
                               const QHostAddress &sender);

  /**
   * @brief 处理设备心跳
   * @param jsonData JSON数据
   * @param sender 发送方地址
   */
  void handleHeartbeat(const QJsonObject &jsonData, const QHostAddress &sender);

  QUdpSocket *m_socket;         // UDP套接字
  QTimer *m_checkTimer;         // 设备状态检查定时器
  QTimer *m_autoDiscoveryTimer; // 自动发现请求定时器
  QMap<QString, DiscoveredDevice>
      m_devices;       // 已发现设备映射表（deviceId -> device）
  quint16 m_port;      // 监听端口
  bool m_running;      // 运行状态
  int m_deviceTimeout; // 设备超时时间（秒）
};
