#include "AddCameraDialog.h"
#include "../controller/DeviceDiscovery.h"
#include "DeviceDiscoveryDialog.h"
#include <QMessageBox>
#include <QRegExpValidator>

AddCameraDialog::AddCameraDialog(const QList<int> &availableIds,
                                 QWidget *parent)
    : QDialog(parent), availableCameraIds(availableIds), selectedCameraId(-1) {
  setupUI();

  // 设置默认值
  if (!availableIds.isEmpty()) {
    selectedCameraId = availableIds.first();
    updateNamePlaceholder();
  }
}

// 固定摄像头ID的构造函数
AddCameraDialog::AddCameraDialog(int fixedCameraId, QWidget *parent)
    : QDialog(parent), availableCameraIds({fixedCameraId}) // 只包含固定的ID
      ,
      selectedCameraId(fixedCameraId) {
  setupUI();

  // 禁用摄像头ID选择下拉框（因为ID已固定）
  if (cameraIdComboBox) {
    cameraIdComboBox->setEnabled(false);
    cameraIdComboBox->setStyleSheet("QComboBox {"
                                    "  border: 1px solid #cccccc;"
                                    "  border-radius: 4px;"
                                    "  padding: 5px;"
                                    "  font-size: 13px;"
                                    "  background-color: #f0f0f0;"
                                    "  color: #666666;"
                                    "}");
  }

  updateNamePlaceholder();
}

AddCameraDialog::~AddCameraDialog() {}

void AddCameraDialog::setupUI() {
  setWindowTitle("添加摄像头");
  setMinimumWidth(500);

  // 创建主布局
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setSpacing(15);
  mainLayout->setContentsMargins(20, 20, 20, 20);

  // 创建表单布局
  QFormLayout *formLayout = new QFormLayout();
  formLayout->setSpacing(12);
  formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

  // 1. 摄像头位置选择
  cameraIdComboBox = new QComboBox(this);
  cameraIdComboBox->setMinimumHeight(30);
  for (int id : availableCameraIds) {
    cameraIdComboBox->addItem(QString("位置 %1").arg(id), id);
  }
  connect(cameraIdComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &AddCameraDialog::onCameraIdChanged);

  QLabel *idLabel = new QLabel("摄像头位置:", this);
  idLabel->setStyleSheet("font-weight: bold; color: #333333;");
  formLayout->addRow(idLabel, cameraIdComboBox);

  // 2. RTSP地址输入
  rtspUrlLineEdit = new QLineEdit(this);
  rtspUrlLineEdit->setMinimumHeight(30);
  rtspUrlLineEdit->setPlaceholderText("rtsp://192.168.1.158/live/0");
  rtspUrlLineEdit->setText("rtsp://192.168.1.158/live/0"); // 默认地址

  QLabel *urlLabel = new QLabel("RTSP地址:", this);
  urlLabel->setStyleSheet("font-weight: bold; color: #333333;");
  formLayout->addRow(urlLabel, rtspUrlLineEdit);

  // 3. 摄像头名称输入
  cameraNameLineEdit = new QLineEdit(this);
  cameraNameLineEdit->setMinimumHeight(30);
  if (!availableCameraIds.isEmpty()) {
    cameraNameLineEdit->setPlaceholderText(
        QString("摄像头 %1").arg(availableCameraIds.first()));
  }

  QLabel *nameLabel = new QLabel("摄像头名称:", this);
  nameLabel->setStyleSheet("font-weight: bold; color: #333333;");
  formLayout->addRow(nameLabel, cameraNameLineEdit);

  mainLayout->addLayout(formLayout);

  // 添加提示信息
  QLabel *hintLabel =
      new QLabel("提示：\n"
                 "• 摄像头位置对应显示网格中的位置编号（1-16）\n"
                 "• RTSP地址格式：rtsp://用户名:密码@IP地址:端口/路径\n"
                 "• 摄像头名称可自定义，留空则使用默认名称",
                 this);
  hintLabel->setStyleSheet("QLabel {"
                           "  color: #666666;"
                           "  font-size: 11px;"
                           "  background-color: #f0f0f0;"
                           "  border: 1px solid #cccccc;"
                           "  border-radius: 4px;"
                           "  padding: 10px;"
                           "  margin-top: 5px;"
                           "}");
  hintLabel->setWordWrap(true);
  mainLayout->addWidget(hintLabel);

  // 添加弹性空间
  mainLayout->addStretch();

  // 创建按钮布局
  QHBoxLayout *buttonLayout = new QHBoxLayout();

  // 自动发现按钮（放在最左边）
  autoDiscoveryButton = new QPushButton("🔍 自动发现", this);
  autoDiscoveryButton->setMinimumSize(100, 32);
  autoDiscoveryButton->setStyleSheet("QPushButton {"
                                     "  background-color: #4CAF50;"
                                     "  color: white;"
                                     "  border: none;"
                                     "  border-radius: 4px;"
                                     "  font-weight: bold;"
                                     "  padding: 5px 15px;"
                                     "}"
                                     "QPushButton:hover {"
                                     "  background-color: #45a049;"
                                     "}"
                                     "QPushButton:pressed {"
                                     "  background-color: #3d8b40;"
                                     "}");
  connect(autoDiscoveryButton, &QPushButton::clicked, this,
          &AddCameraDialog::onAutoDiscoveryClicked);
  buttonLayout->addWidget(autoDiscoveryButton);

  buttonLayout->addStretch();

  cancelButton = new QPushButton("取消", this);
  cancelButton->setMinimumSize(80, 32);
  cancelButton->setStyleSheet("QPushButton {"
                              "  background-color: #cccccc;"
                              "  color: #333333;"
                              "  border: none;"
                              "  border-radius: 4px;"
                              "  font-weight: bold;"
                              "  padding: 5px 15px;"
                              "}"
                              "QPushButton:hover {"
                              "  background-color: #bbbbbb;"
                              "}"
                              "QPushButton:pressed {"
                              "  background-color: #aaaaaa;"
                              "}");

  okButton = new QPushButton("确定", this);
  okButton->setMinimumSize(80, 32);
  okButton->setStyleSheet("QPushButton {"
                          "  background-color: #0078d4;"
                          "  color: white;"
                          "  border: none;"
                          "  border-radius: 4px;"
                          "  font-weight: bold;"
                          "  padding: 5px 15px;"
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
  okButton->setDefault(true);

  buttonLayout->addWidget(cancelButton);
  buttonLayout->addSpacing(10);
  buttonLayout->addWidget(okButton);

  mainLayout->addLayout(buttonLayout);

  // 连接按钮信号
  connect(okButton, &QPushButton::clicked, this, &AddCameraDialog::onAccepted);
  connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

  // 设置对话框样式
  setStyleSheet("QDialog {"
                "  background-color: white;"
                "}"
                "QLineEdit, QComboBox {"
                "  border: 1px solid #cccccc;"
                "  border-radius: 4px;"
                "  padding: 5px;"
                "  font-size: 13px;"
                "}"
                "QLineEdit:focus, QComboBox:focus {"
                "  border: 2px solid #0078d4;"
                "}"
                "QComboBox::drop-down {"
                "  border: none;"
                "  width: 20px;"
                "}"
                "QComboBox::down-arrow {"
                "  image: none;"
                "  border-left: 5px solid transparent;"
                "  border-right: 5px solid transparent;"
                "  border-top: 6px solid #666666;"
                "  margin-right: 5px;"
                "}");
}

void AddCameraDialog::onCameraIdChanged(int index) {
  if (index >= 0 && index < availableCameraIds.size()) {
    selectedCameraId = availableCameraIds[index];
    updateNamePlaceholder();
  }
}

void AddCameraDialog::updateNamePlaceholder() {
  if (cameraNameLineEdit && selectedCameraId > 0) {
    cameraNameLineEdit->setPlaceholderText(
        QString("摄像头 %1").arg(selectedCameraId));
  }
}

void AddCameraDialog::onAccepted() {
  // 验证输入
  rtspUrl = rtspUrlLineEdit->text().trimmed();

  if (rtspUrl.isEmpty()) {
    QMessageBox::warning(this, "输入错误", "请输入RTSP地址！");
    rtspUrlLineEdit->setFocus();
    return;
  }

  // 简单验证RTSP地址格式
  if (!rtspUrl.startsWith("rtsp://", Qt::CaseInsensitive)) {
    QMessageBox::warning(this, "格式错误", "RTSP地址必须以 rtsp:// 开头！");
    rtspUrlLineEdit->setFocus();
    return;
  }

  // 获取摄像头名称（如果为空，使用默认名称）
  cameraName = cameraNameLineEdit->text().trimmed();
  if (cameraName.isEmpty()) {
    cameraName = QString("摄像头 %1").arg(selectedCameraId);
  }

  // 接受对话框
  accept();
}

int AddCameraDialog::getSelectedCameraId() const { return selectedCameraId; }

QString AddCameraDialog::getRtspUrl() const { return rtspUrl; }

QString AddCameraDialog::getCameraName() const { return cameraName; }

void AddCameraDialog::onAutoDiscoveryClicked() {
  DeviceDiscoveryDialog discoveryDialog(this);

  // 连接设备选择信号
  connect(&discoveryDialog, &DeviceDiscoveryDialog::deviceSelected, this,
          &AddCameraDialog::onDeviceSelected);

  discoveryDialog.exec();
}

void AddCameraDialog::onDeviceSelected(const DiscoveredDevice &device) {
  // 将发现的设备信息填充到表单中
  rtspUrlLineEdit->setText(device.rtspUrl);

  // 如果设备有名称，填充到名称输入框
  if (!device.deviceName.isEmpty()) {
    cameraNameLineEdit->setText(device.deviceName);
  }

  // 显示提示信息
  QMessageBox::information(
      this, "设备已选择",
      QString("已选择设备：%1\nIP地址：%2\nRTSP地址：%3\n\n请点击确定"
              "添加此摄像头。")
          .arg(device.deviceName)
          .arg(device.ipAddress)
          .arg(device.rtspUrl));
}
