#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QList>

class AddCameraDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddCameraDialog(const QList<int>& availableIds, QWidget *parent = nullptr);
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

private:
    void setupUI();
    void updateNamePlaceholder();

    QComboBox* cameraIdComboBox;
    QLineEdit* rtspUrlLineEdit;
    QLineEdit* cameraNameLineEdit;
    QPushButton* okButton;
    QPushButton* cancelButton;
    
    QList<int> availableCameraIds;
    int selectedCameraId;
    QString rtspUrl;
    QString cameraName;
};
