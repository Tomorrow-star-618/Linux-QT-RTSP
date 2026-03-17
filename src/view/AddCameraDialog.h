#pragma once

#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPushButton>
#include <QVBoxLayout>

// 前向声明
struct DiscoveredDevice;

class AddCameraDialog : public QDialog {
  Q_OBJECT

public:
  explicit AddCameraDialog(const QList<int> &availableIds,
                           QWidget *parent = nullptr);
  // 构造函数：固定摄像头ID（不可选择）
  explicit AddCameraDialog(int fixedCameraId, QWidget *parent = nullptr);
  ~AddCameraDialog();

  // 获取用户输入的信息
  int getSelectedCameraId() const;
  QString getRtspUrl() const;
  QString getCameraName() const;

private slots:
  void onCameraIdChanged(int index);
  void onAccepted();
  void onAutoDiscoveryClicked();                         // 自动发现按钮点击
  void onDeviceSelected(const DiscoveredDevice &device); // 处理设备选择

private:
  void setupUI();
  void updateNamePlaceholder();

  QComboBox *cameraIdComboBox;
  QLineEdit *rtspUrlLineEdit;
  QLineEdit *cameraNameLineEdit;
  QPushButton *okButton;
  QPushButton *cancelButton;
  QPushButton *autoDiscoveryButton; // 自动发现按钮

  QList<int> availableCameraIds;
  int selectedCameraId;
  QString rtspUrl;
  QString cameraName;
};
