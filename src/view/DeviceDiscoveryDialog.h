#pragma once

#include "../controller/DeviceDiscovery.h"
#include <QDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>


/**
 * @brief 设备发现对话框
 *
 * 用于展示通过UDP广播发现的网络摄像头设备列表，
 * 支持用户选择设备进行快速接入。
 */
class DeviceDiscoveryDialog : public QDialog {
  Q_OBJECT

public:
  explicit DeviceDiscoveryDialog(QWidget *parent = nullptr);
  ~DeviceDiscoveryDialog();

  /**
   * @brief 获取用户选择的设备
   * @return 选择的设备信息
   */
  DiscoveredDevice getSelectedDevice() const { return m_selectedDevice; }

  /**
   * @brief 检查是否有选择设备
   * @return 是否选择了设备
   */
  bool hasSelectedDevice() const {
    return !m_selectedDevice.deviceId.isEmpty();
  }

signals:
  /**
   * @brief 设备选择确认信号
   * @param device 被选择的设备
   */
  void deviceSelected(const DiscoveredDevice &device);

private slots:
  void onDeviceDiscovered(const DiscoveredDevice &device);
  void onDeviceUpdated(const DiscoveredDevice &device);
  void onDeviceOffline(const QString &deviceId);
  void onRefreshClicked();
  void onConnectClicked();
  void onTableSelectionChanged();
  void onTableDoubleClicked(int row, int column);
  void updateScanningAnimation();

private:
  void setupUI();
  void setupDiscovery();
  void updateDeviceInTable(const DiscoveredDevice &device);
  int findDeviceRow(const QString &deviceId);
  void setDeviceRowOnline(int row, bool online);

  // UI组件
  QLabel *titleLabel;
  QLabel *statusLabel;
  QProgressBar *scanningProgress;
  QTableWidget *deviceTable;
  QPushButton *refreshButton;
  QPushButton *connectButton;
  QPushButton *cancelButton;

  // 设备发现
  DeviceDiscovery *m_discovery;
  DiscoveredDevice m_selectedDevice;

  // 动画相关
  QTimer *m_animationTimer;
  int m_animationStep;
};
