#include "DeviceDiscoveryDialog.h"
#include <QDebug>
#include <QMessageBox>

DeviceDiscoveryDialog::DeviceDiscoveryDialog(QWidget *parent)
    : QDialog(parent), m_discovery(nullptr), m_animationTimer(nullptr),
      m_animationStep(0) {
  setupUI();
  setupDiscovery();
}

DeviceDiscoveryDialog::~DeviceDiscoveryDialog() {
  if (m_discovery) {
    m_discovery->stopDiscovery();
  }
  if (m_animationTimer) {
    m_animationTimer->stop();
  }
}

void DeviceDiscoveryDialog::setupUI() {
  setWindowTitle("设备自动发现");
  setMinimumSize(700, 450);
  resize(750, 500);

  // 主布局
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setSpacing(15);
  mainLayout->setContentsMargins(20, 20, 20, 20);

  // 标题和状态区域
  QHBoxLayout *headerLayout = new QHBoxLayout();

  titleLabel = new QLabel("🔍 搜索局域网中的摄像头设备...", this);
  titleLabel->setStyleSheet("QLabel {"
                            "  font-size: 16px;"
                            "  font-weight: bold;"
                            "  color: #333333;"
                            "}");
  headerLayout->addWidget(titleLabel);
  headerLayout->addStretch();

  statusLabel = new QLabel("正在扫描...", this);
  statusLabel->setStyleSheet("QLabel {"
                             "  font-size: 12px;"
                             "  color: #666666;"
                             "}");
  headerLayout->addWidget(statusLabel);

  mainLayout->addLayout(headerLayout);

  // 扫描进度条
  scanningProgress = new QProgressBar(this);
  scanningProgress->setRange(0, 0); // 无限循环动画
  scanningProgress->setTextVisible(false);
  scanningProgress->setFixedHeight(4);
  scanningProgress->setStyleSheet(
      "QProgressBar {"
      "  background-color: #e0e0e0;"
      "  border: none;"
      "  border-radius: 2px;"
      "}"
      "QProgressBar::chunk {"
      "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
      "    stop:0 #0078d4, stop:0.5 #00a2ff, stop:1 #0078d4);"
      "  border-radius: 2px;"
      "}");
  mainLayout->addWidget(scanningProgress);

  // 设备列表表格
  deviceTable = new QTableWidget(this);
  deviceTable->setColumnCount(6);
  deviceTable->setHorizontalHeaderLabels(
      {"状态", "设备名称", "IP地址", "RTSP地址", "型号", "最后响应"});

  // 设置列宽
  deviceTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
  deviceTable->setColumnWidth(0, 50);
  deviceTable->horizontalHeader()->setSectionResizeMode(1,
                                                        QHeaderView::Stretch);
  deviceTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
  deviceTable->setColumnWidth(2, 120);
  deviceTable->horizontalHeader()->setSectionResizeMode(3,
                                                        QHeaderView::Stretch);
  deviceTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
  deviceTable->setColumnWidth(4, 100);
  deviceTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
  deviceTable->setColumnWidth(5, 100);

  // 表格样式
  deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  deviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
  deviceTable->setAlternatingRowColors(true);
  deviceTable->setShowGrid(false);
  deviceTable->verticalHeader()->setVisible(false);
  deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

  deviceTable->setStyleSheet("QTableWidget {"
                             "  background-color: white;"
                             "  border: 1px solid #cccccc;"
                             "  border-radius: 4px;"
                             "  font-size: 13px;"
                             "}"
                             "QTableWidget::item {"
                             "  padding: 8px;"
                             "  border-bottom: 1px solid #eeeeee;"
                             "}"
                             "QTableWidget::item:selected {"
                             "  background-color: #0078d4;"
                             "  color: white;"
                             "}"
                             "QTableWidget::item:alternate {"
                             "  background-color: #f8f8f8;"
                             "}"
                             "QHeaderView::section {"
                             "  background-color: #f0f0f0;"
                             "  color: #333333;"
                             "  font-weight: bold;"
                             "  padding: 8px;"
                             "  border: none;"
                             "  border-bottom: 2px solid #cccccc;"
                             "}");

  connect(deviceTable, &QTableWidget::itemSelectionChanged, this,
          &DeviceDiscoveryDialog::onTableSelectionChanged);
  connect(deviceTable, &QTableWidget::cellDoubleClicked, this,
          &DeviceDiscoveryDialog::onTableDoubleClicked);

  mainLayout->addWidget(deviceTable);

  // 提示信息
  QLabel *hintLabel = new QLabel(
      "💡 "
      "提示：确保摄像头设备与本机在同一局域网内，且设备支持UDP广播发现协议。\n"
      "       双击设备可快速连接，或选择设备后点击连接"
      "按钮。",
      this);
  hintLabel->setStyleSheet("QLabel {"
                           "  color: #666666;"
                           "  font-size: 11px;"
                           "  background-color: #f5f5f5;"
                           "  border: 1px solid #e0e0e0;"
                           "  border-radius: 4px;"
                           "  padding: 10px;"
                           "}");
  hintLabel->setWordWrap(true);
  mainLayout->addWidget(hintLabel);

  // 按钮区域
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();

  refreshButton = new QPushButton("🔄 刷新", this);
  refreshButton->setMinimumSize(90, 36);
  refreshButton->setStyleSheet("QPushButton {"
                               "  background-color: #f0f0f0;"
                               "  color: #333333;"
                               "  border: 1px solid #cccccc;"
                               "  border-radius: 4px;"
                               "  font-weight: bold;"
                               "  padding: 8px 16px;"
                               "}"
                               "QPushButton:hover {"
                               "  background-color: #e0e0e0;"
                               "}"
                               "QPushButton:pressed {"
                               "  background-color: #d0d0d0;"
                               "}");
  connect(refreshButton, &QPushButton::clicked, this,
          &DeviceDiscoveryDialog::onRefreshClicked);
  buttonLayout->addWidget(refreshButton);

  buttonLayout->addSpacing(10);

  cancelButton = new QPushButton("取消", this);
  cancelButton->setMinimumSize(90, 36);
  cancelButton->setStyleSheet("QPushButton {"
                              "  background-color: #cccccc;"
                              "  color: #333333;"
                              "  border: none;"
                              "  border-radius: 4px;"
                              "  font-weight: bold;"
                              "  padding: 8px 16px;"
                              "}"
                              "QPushButton:hover {"
                              "  background-color: #bbbbbb;"
                              "}"
                              "QPushButton:pressed {"
                              "  background-color: #aaaaaa;"
                              "}");
  connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
  buttonLayout->addWidget(cancelButton);

  buttonLayout->addSpacing(10);

  connectButton = new QPushButton("✓ 连接", this);
  connectButton->setMinimumSize(90, 36);
  connectButton->setEnabled(false);
  connectButton->setStyleSheet("QPushButton {"
                               "  background-color: #0078d4;"
                               "  color: white;"
                               "  border: none;"
                               "  border-radius: 4px;"
                               "  font-weight: bold;"
                               "  padding: 8px 16px;"
                               "}"
                               "QPushButton:hover {"
                               "  background-color: #006cc1;"
                               "}"
                               "QPushButton:pressed {"
                               "  background-color: #005a9e;"
                               "}"
                               "QPushButton:disabled {"
                               "  background-color: #cccccc;"
                               "  color: #666666;"
                               "}");
  connect(connectButton, &QPushButton::clicked, this,
          &DeviceDiscoveryDialog::onConnectClicked);
  buttonLayout->addWidget(connectButton);

  mainLayout->addLayout(buttonLayout);

  // 设置对话框样式
  setStyleSheet("QDialog {"
                "  background-color: white;"
                "}");

  // 动画定时器
  m_animationTimer = new QTimer(this);
  m_animationTimer->setInterval(100);
  connect(m_animationTimer, &QTimer::timeout, this,
          &DeviceDiscoveryDialog::updateScanningAnimation);
  m_animationTimer->start();
}

void DeviceDiscoveryDialog::setupDiscovery() {
  m_discovery = new DeviceDiscovery(this);

  connect(m_discovery, &DeviceDiscovery::deviceDiscovered, this,
          &DeviceDiscoveryDialog::onDeviceDiscovered);
  connect(m_discovery, &DeviceDiscovery::deviceUpdated, this,
          &DeviceDiscoveryDialog::onDeviceUpdated);
  connect(m_discovery, &DeviceDiscovery::deviceOffline, this,
          &DeviceDiscoveryDialog::onDeviceOffline);
  connect(m_discovery, &DeviceDiscovery::errorOccurred,
          [this](const QString &error) {
            statusLabel->setText("错误: " + error);
            statusLabel->setStyleSheet(
                "QLabel { font-size: 12px; color: #d32f2f; }");
          });

  // 设置自动发现间隔（每10秒发送一次）
  m_discovery->setAutoDiscoveryInterval(10000);

  // 启动发现服务
  if (!m_discovery->startDiscovery()) {
    QMessageBox::warning(
        this, "启动失败",
        "无法启动设备发现服务！\n请检查网络连接和防火墙设置。");
  }
}

void DeviceDiscoveryDialog::onDeviceDiscovered(const DiscoveredDevice &device) {
  qDebug() << "DeviceDiscoveryDialog: 发现新设备 -" << device.deviceName;
  updateDeviceInTable(device);

  int count = deviceTable->rowCount();
  statusLabel->setText(QString("已发现 %1 台设备").arg(count));
  titleLabel->setText(QString("🔍 已发现 %1 台摄像头设备").arg(count));
}

void DeviceDiscoveryDialog::onDeviceUpdated(const DiscoveredDevice &device) {
  updateDeviceInTable(device);
}

void DeviceDiscoveryDialog::onDeviceOffline(const QString &deviceId) {
  int row = findDeviceRow(deviceId);
  if (row >= 0) {
    setDeviceRowOnline(row, false);
  }
}

void DeviceDiscoveryDialog::onRefreshClicked() {
  // 清除现有设备
  deviceTable->setRowCount(0);
  m_discovery->clearDevices();

  // 重新发送发现请求
  m_discovery->sendDiscoveryRequest();

  statusLabel->setText("正在扫描...");
  titleLabel->setText("🔍 搜索局域网中的摄像头设备...");

  qDebug() << "DeviceDiscoveryDialog: 刷新设备列表";
}

void DeviceDiscoveryDialog::onConnectClicked() {
  QList<QTableWidgetItem *> selected = deviceTable->selectedItems();
  if (selected.isEmpty()) {
    QMessageBox::information(this, "提示", "请先选择一个设备！");
    return;
  }

  int row = selected.first()->row();
  QString deviceId = deviceTable->item(row, 0)->data(Qt::UserRole).toString();

  m_selectedDevice = m_discovery->getDevice(deviceId);

  if (!m_selectedDevice.isOnline) {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "设备离线", "该设备当前处于离线状态，是否仍要尝试连接？",
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
      return;
    }
  }

  // 发送连接请求给设备（TCP端口固定8890）
  const quint16 TCP_PORT = 8890;
  if (m_discovery->sendConnectionRequest(m_selectedDevice, TCP_PORT)) {
    qDebug()
        << "DeviceDiscoveryDialog: 已向设备发送连接请求，等待设备建立TCP连接";
  } else {
    QMessageBox::warning(this, "连接失败",
                         "发送连接请求失败！请检查网络连接。");
    return;
  }

  emit deviceSelected(m_selectedDevice);
  accept();
}

void DeviceDiscoveryDialog::onTableSelectionChanged() {
  bool hasSelection = !deviceTable->selectedItems().isEmpty();
  connectButton->setEnabled(hasSelection);
}

void DeviceDiscoveryDialog::onTableDoubleClicked(int row, int column) {
  Q_UNUSED(column)

  QString deviceId = deviceTable->item(row, 0)->data(Qt::UserRole).toString();
  m_selectedDevice = m_discovery->getDevice(deviceId);

  if (!m_selectedDevice.deviceId.isEmpty()) {
    // 发送连接请求给设备
    const quint16 TCP_PORT = 8890;
    if (m_discovery->sendConnectionRequest(m_selectedDevice, TCP_PORT)) {
      qDebug() << "DeviceDiscoveryDialog: 已向设备发送连接请求（双击）";
    }

    emit deviceSelected(m_selectedDevice);
    accept();
  }
}

void DeviceDiscoveryDialog::updateScanningAnimation() {
  m_animationStep = (m_animationStep + 1) % 4;
  QString dots;
  for (int i = 0; i < m_animationStep; ++i) {
    dots += ".";
  }

  if (deviceTable->rowCount() == 0) {
    statusLabel->setText("正在扫描" + dots);
  }
}

void DeviceDiscoveryDialog::updateDeviceInTable(
    const DiscoveredDevice &device) {
  int row = findDeviceRow(device.deviceId);

  if (row < 0) {
    // 新设备，添加新行
    row = deviceTable->rowCount();
    deviceTable->insertRow(row);

    // 创建所有单元格
    for (int col = 0; col < 6; ++col) {
      QTableWidgetItem *item = new QTableWidgetItem();
      item->setTextAlignment(Qt::AlignCenter);
      deviceTable->setItem(row, col, item);
    }

    // 存储设备ID
    deviceTable->item(row, 0)->setData(Qt::UserRole, device.deviceId);
  }

  // 更新单元格内容
  setDeviceRowOnline(row, device.isOnline);

  deviceTable->item(row, 1)->setText(device.deviceName);
  deviceTable->item(row, 1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  deviceTable->item(row, 2)->setText(device.ipAddress);

  deviceTable->item(row, 3)->setText(device.rtspUrl);
  deviceTable->item(row, 3)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  deviceTable->item(row, 3)->setToolTip(device.rtspUrl);

  QString modelInfo = device.model.isEmpty() ? "-" : device.model;
  if (!device.manufacturer.isEmpty()) {
    modelInfo = device.manufacturer + " " + modelInfo;
  }
  deviceTable->item(row, 4)->setText(modelInfo);

  deviceTable->item(row, 5)->setText(device.lastSeen.toString("HH:mm:ss"));
}

int DeviceDiscoveryDialog::findDeviceRow(const QString &deviceId) {
  for (int row = 0; row < deviceTable->rowCount(); ++row) {
    QTableWidgetItem *item = deviceTable->item(row, 0);
    if (item && item->data(Qt::UserRole).toString() == deviceId) {
      return row;
    }
  }
  return -1;
}

void DeviceDiscoveryDialog::setDeviceRowOnline(int row, bool online) {
  QTableWidgetItem *statusItem = deviceTable->item(row, 0);
  if (statusItem) {
    if (online) {
      statusItem->setText("🟢");
      statusItem->setToolTip("在线");
    } else {
      statusItem->setText("🔴");
      statusItem->setToolTip("离线");
    }
  }

  // 设置行的整体颜色
  QColor textColor = online ? QColor("#333333") : QColor("#999999");
  for (int col = 1; col < deviceTable->columnCount(); ++col) {
    QTableWidgetItem *item = deviceTable->item(row, col);
    if (item) {
      item->setForeground(textColor);
    }
  }
}
