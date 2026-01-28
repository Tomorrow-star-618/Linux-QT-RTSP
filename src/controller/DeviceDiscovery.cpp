#include "DeviceDiscovery.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>

DeviceDiscovery::DeviceDiscovery(QObject *parent)
    : QObject(parent), m_socket(nullptr), m_checkTimer(nullptr),
      m_autoDiscoveryTimer(nullptr), m_port(DEFAULT_DISCOVERY_PORT),
      m_running(false), m_deviceTimeout(30) {
  // 创建定时器
  m_checkTimer = new QTimer(this);
  m_checkTimer->setInterval(5000); // 每5秒检查一次设备状态
  connect(m_checkTimer, &QTimer::timeout, this,
          &DeviceDiscovery::onCheckDevicesTimeout);

  m_autoDiscoveryTimer = new QTimer(this);
  connect(m_autoDiscoveryTimer, &QTimer::timeout, this,
          &DeviceDiscovery::onAutoDiscoveryTimeout);
}

DeviceDiscovery::~DeviceDiscovery() { stopDiscovery(); }

bool DeviceDiscovery::startDiscovery(quint16 port) {
  if (m_running) {
    qDebug() << "DeviceDiscovery: 服务已经在运行中";
    return true;
  }

  m_port = port;

  // 创建UDP套接字
  if (m_socket) {
    delete m_socket;
  }
  m_socket = new QUdpSocket(this);

  // 绑定到指定端口，允许端口复用
  if (!m_socket->bind(QHostAddress::AnyIPv4, m_port,
                      QUdpSocket::ShareAddress |
                          QUdpSocket::ReuseAddressHint)) {
    QString error = QString("DeviceDiscovery: 无法绑定到端口 %1 - %2")
                        .arg(m_port)
                        .arg(m_socket->errorString());
    qDebug() << error;
    emit errorOccurred(error);
    delete m_socket;
    m_socket = nullptr;
    return false;
  }

  // 连接信号槽
  connect(m_socket, &QUdpSocket::readyRead, this,
          &DeviceDiscovery::onReadyRead);

  // 启动定时器
  m_checkTimer->start();

  m_running = true;
  qDebug() << "DeviceDiscovery: 设备发现服务已启动，监听端口:" << m_port;

  // 立即发送一次发现请求
  sendDiscoveryRequest();

  return true;
}

void DeviceDiscovery::stopDiscovery() {
  if (!m_running) {
    return;
  }

  m_running = false;

  // 停止定时器
  if (m_checkTimer) {
    m_checkTimer->stop();
  }
  if (m_autoDiscoveryTimer) {
    m_autoDiscoveryTimer->stop();
  }

  // 关闭套接字
  if (m_socket) {
    m_socket->close();
    delete m_socket;
    m_socket = nullptr;
  }

  qDebug() << "DeviceDiscovery: 设备发现服务已停止";
}

void DeviceDiscovery::sendDiscoveryRequest() {
  if (!m_socket || !m_running) {
    qDebug() << "DeviceDiscovery: 无法发送发现请求，服务未运行";
    return;
  }

  // 构建发现请求JSON
  QJsonObject request;
  request["type"] = "discovery_request";
  request["version"] = "1.0";
  request["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

  QJsonDocument doc(request);
  QByteArray data = doc.toJson(QJsonDocument::Compact);

  // 向广播地址发送
  QHostAddress broadcastAddr(QHostAddress::Broadcast);
  qint64 sent = m_socket->writeDatagram(data, broadcastAddr, m_port);

  if (sent == -1) {
    qDebug() << "DeviceDiscovery: 发送广播失败 -" << m_socket->errorString();
  } else {
    qDebug() << "DeviceDiscovery: 已发送设备发现广播请求 (" << sent << "字节)";
  }

  // 也尝试向各个网络接口的广播地址发送
  for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
    if (iface.flags() & QNetworkInterface::IsUp &&
        iface.flags() & QNetworkInterface::IsRunning &&
        !(iface.flags() & QNetworkInterface::IsLoopBack)) {

      for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
        if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
          QHostAddress broadcast = entry.broadcast();
          if (!broadcast.isNull() && broadcast != broadcastAddr) {
            m_socket->writeDatagram(data, broadcast, m_port);
            qDebug() << "DeviceDiscovery: 向" << broadcast.toString()
                     << "发送发现请求";
          }
        }
      }
    }
  }
}

QList<DiscoveredDevice> DeviceDiscovery::getDiscoveredDevices() const {
  return m_devices.values();
}

DiscoveredDevice DeviceDiscovery::getDevice(const QString &deviceId) const {
  return m_devices.value(deviceId, DiscoveredDevice());
}

void DeviceDiscovery::clearDevices() {
  m_devices.clear();
  qDebug() << "DeviceDiscovery: 已清除所有发现的设备";
}

void DeviceDiscovery::setAutoDiscoveryInterval(int msec) {
  if (msec <= 0) {
    m_autoDiscoveryTimer->stop();
    qDebug() << "DeviceDiscovery: 自动发现已禁用";
  } else {
    m_autoDiscoveryTimer->setInterval(msec);
    if (m_running) {
      m_autoDiscoveryTimer->start();
    }
    qDebug() << "DeviceDiscovery: 自动发现间隔设置为" << msec << "毫秒";
  }
}

void DeviceDiscovery::onReadyRead() {
  while (m_socket && m_socket->hasPendingDatagrams()) {
    QByteArray buffer;
    buffer.resize(m_socket->pendingDatagramSize());
    QHostAddress sender;
    quint16 senderPort;

    qint64 size = m_socket->readDatagram(buffer.data(), buffer.size(), &sender,
                                         &senderPort);

    if (size > 0) {
      buffer.resize(size);
      parseMessage(buffer, sender, senderPort);
    }
  }
}

void DeviceDiscovery::onCheckDevicesTimeout() {
  QDateTime now = QDateTime::currentDateTime();
  QStringList offlineDevices;

  for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
    if (it->lastSeen.secsTo(now) > m_deviceTimeout) {
      if (it->isOnline) {
        it->isOnline = false;
        offlineDevices.append(it->deviceId);
        qDebug() << "DeviceDiscovery: 设备离线 -" << it->deviceName << "("
                 << it->deviceId << ")";
      }
    }
  }

  // 发送离线信号
  for (const QString &deviceId : offlineDevices) {
    emit deviceOffline(deviceId);
  }
}

void DeviceDiscovery::onAutoDiscoveryTimeout() { sendDiscoveryRequest(); }

void DeviceDiscovery::parseMessage(const QByteArray &data,
                                   const QHostAddress &sender,
                                   quint16 senderPort) {
  Q_UNUSED(senderPort)

  // 尝试解析JSON
  QJsonParseError error;
  QJsonDocument doc = QJsonDocument::fromJson(data, &error);

  if (error.error != QJsonParseError::NoError) {
    qDebug() << "DeviceDiscovery: 解析JSON失败 -" << error.errorString();
    qDebug() << "DeviceDiscovery: 原始数据:" << data;
    return;
  }

  QJsonObject jsonData = doc.object();
  QString type = jsonData.value("type").toString();

  if (type == "discovery_response") {
    handleDiscoveryResponse(jsonData, sender);
  } else if (type == "heartbeat") {
    handleHeartbeat(jsonData, sender);
  } else if (type == "discovery_request") {
    // 忽略自己发出的发现请求
    qDebug() << "DeviceDiscovery: 收到发现请求（忽略）来自"
             << sender.toString();
  } else {
    qDebug() << "DeviceDiscovery: 收到未知类型消息:" << type;
  }
}

void DeviceDiscovery::handleDiscoveryResponse(const QJsonObject &jsonData,
                                              const QHostAddress &sender) {
  QString deviceId = jsonData.value("device_id").toString();
  if (deviceId.isEmpty()) {
    qDebug() << "DeviceDiscovery: 收到无效的设备响应（缺少device_id）";
    return;
  }

  bool isNewDevice = !m_devices.contains(deviceId);

  DiscoveredDevice &device = m_devices[deviceId];
  device.deviceId = deviceId;
  device.deviceName =
      jsonData.value("device_name")
          .toString(QString("未知设备_%1").arg(deviceId.left(8)));

  // 处理IP地址：优先使用UDP包的来源IP，并确保是IPv4格式
  QString senderIp = sender.toString();
  if (senderIp.startsWith("::ffff:")) {
    senderIp = senderIp.mid(7);
  }
  device.ipAddress = senderIp; // 忽略JSON里的IP，直接用物理来源IP，更加可靠

  device.rtspPort = jsonData.value("rtsp_port").toInt(554);

  // 解析RTSP URL
  QString rtspUrl = jsonData.value("rtsp_url").toString();
  if (rtspUrl.isEmpty()) {
    // 如果没有提供完整的RTSP URL，则根据IP地址构造
    // 注意：如果端口是554，不要拼接到URL中，某些特定的流媒体库对格式敏感
    if (device.rtspPort == 554) {
      rtspUrl = QString("rtsp://%1/live/0").arg(device.ipAddress);
    } else {
      rtspUrl = QString("rtsp://%1:%2/live/0")
                    .arg(device.ipAddress)
                    .arg(device.rtspPort);
    }
  }
  device.rtspUrl = rtspUrl;

  device.manufacturer = jsonData.value("manufacturer").toString();
  device.model = jsonData.value("model").toString();
  device.firmwareVersion = jsonData.value("firmware_version").toString();
  device.lastSeen = QDateTime::currentDateTime();
  device.isOnline = true;

  qDebug() << "DeviceDiscovery:" << (isNewDevice ? "发现新设备" : "设备更新")
           << "-" << device.deviceName << "(" << device.ipAddress << ")"
           << "RTSP:" << device.rtspUrl;

  if (isNewDevice) {
    emit deviceDiscovered(device);
  } else {
    emit deviceUpdated(device);
  }
}

void DeviceDiscovery::handleHeartbeat(const QJsonObject &jsonData,
                                      const QHostAddress &sender) {
  QString deviceId = jsonData.value("device_id").toString();
  if (deviceId.isEmpty()) {
    return;
  }

  if (m_devices.contains(deviceId)) {
    DiscoveredDevice &device = m_devices[deviceId];
    bool wasOffline = !device.isOnline;
    device.lastSeen = QDateTime::currentDateTime();
    device.isOnline = true;

    if (wasOffline) {
      qDebug() << "DeviceDiscovery: 设备重新上线 -" << device.deviceName;
      emit deviceUpdated(device);
    }
  } else {
    // 收到未知设备的心跳，发送一次发现请求
    qDebug() << "DeviceDiscovery: 收到未知设备心跳，来自" << sender.toString();
    sendDiscoveryRequest();
  }
}

bool DeviceDiscovery::sendConnectionRequest(const DiscoveredDevice &device,
                                            quint16 tcpPort) {
  return sendConnectionRequest(device.ipAddress, tcpPort);
}

bool DeviceDiscovery::sendConnectionRequest(const QString &deviceIp,
                                            quint16 tcpPort) {
  if (!m_socket || !m_running) {
    qDebug() << "DeviceDiscovery: 无法发送连接请求，服务未运行";
    return false;
  }

  // 获取本机IP地址 - 优先选择与目标设备在同一网段的IP
  QString localIp;
  QHostAddress targetAddr(deviceIp);
  QList<QString> candidateIps;

  for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
    // 跳过回环接口和未启用的接口
    if (!(iface.flags() & QNetworkInterface::IsUp) ||
        !(iface.flags() & QNetworkInterface::IsRunning) ||
        (iface.flags() & QNetworkInterface::IsLoopBack)) {
      continue;
    }

    // 过滤虚拟网卡（常见的虚拟网卡名称）
    QString ifaceName = iface.name().toLower();
    if (ifaceName.contains("vmware") || ifaceName.contains("virtualbox") ||
        ifaceName.contains("vethernet") || ifaceName.contains("hyper-v") ||
        ifaceName.contains("docker") || ifaceName.contains("vbox")) {
      qDebug() << "DeviceDiscovery: 跳过虚拟网卡:" << iface.name();
      continue;
    }

    for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
      if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol) {
        continue;
      }

      QString ip = entry.ip().toString();
      quint32 ipAddr = entry.ip().toIPv4Address();
      quint32 netmask = entry.netmask().toIPv4Address();
      quint32 targetIp = targetAddr.toIPv4Address();

      // 检查是否在同一网段
      if ((ipAddr & netmask) == (targetIp & netmask)) {
        qDebug() << "DeviceDiscovery: 找到同网段IP:" << ip
                 << "网卡:" << iface.name();
        localIp = ip;
        break;
      }

      // 收集候选IP（私有网段优先）
      if (ip.startsWith("192.168.") || ip.startsWith("10.") ||
          ip.startsWith("172.16.") || ip.startsWith("172.17.") ||
          ip.startsWith("172.18.") || ip.startsWith("172.19.") ||
          ip.startsWith("172.20.") || ip.startsWith("172.21.") ||
          ip.startsWith("172.22.") || ip.startsWith("172.23.") ||
          ip.startsWith("172.24.") || ip.startsWith("172.25.") ||
          ip.startsWith("172.26.") || ip.startsWith("172.27.") ||
          ip.startsWith("172.28.") || ip.startsWith("172.29.") ||
          ip.startsWith("172.30.") || ip.startsWith("172.31.")) {
        candidateIps.append(ip);
      }
    }

    if (!localIp.isEmpty()) {
      break;
    }
  }

  // 如果没有找到同网段的IP，使用候选IP列表中的第一个
  if (localIp.isEmpty() && !candidateIps.isEmpty()) {
    localIp = candidateIps.first();
    qDebug() << "DeviceDiscovery: 未找到同网段IP，使用候选IP:" << localIp;
  }

  if (localIp.isEmpty()) {
    qDebug() << "DeviceDiscovery: 无法获取本机IP地址";
    return false;
  }

  // 构建连接请求JSON
  QJsonObject request;
  request["type"] = "connection_request";
  request["version"] = "1.0";
  request["host_ip"] = localIp;  // 上位机IP地址
  request["tcp_port"] = tcpPort; // 上位机TCP监听端口
  request["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

  QJsonDocument doc(request);
  QByteArray data = doc.toJson(QJsonDocument::Compact);

  // 向目标设备IP发送（单播）
  qint64 sent = m_socket->writeDatagram(data, targetAddr, m_port);

  if (sent == -1) {
    qDebug() << "DeviceDiscovery: 发送连接请求失败 -"
             << m_socket->errorString();
    return false;
  } else {
    qDebug() << "DeviceDiscovery: 已向" << deviceIp << "发送连接请求";
    qDebug() << "  上位机IP:" << localIp << "TCP端口:" << tcpPort;
    return true;
  }
}
