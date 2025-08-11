#include "controller.h"
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>
#include "Picture.h"
#include "Tcpserver.h" // Added for Tcpserver
#include "common.h"
Controller::Controller(Model* model, View* view, QObject* parent)
    : QObject(parent), m_model(model), m_view(view)
{
    // 绑定主功能按键点击事件
    QList<QPushButton*> buttons = m_view->getTabButtons();
    for(QPushButton* button : buttons) {
        connect(button, &QPushButton::clicked, this, &Controller::ButtonClickedHandler);
    }
    // 绑定云台按键点击事件
    QList<QPushButton*> servoBtns = m_view->getServoButtons();
    for(QPushButton* btn : servoBtns) {
        connect(btn, &QPushButton::clicked, this, &Controller::ServoButtonClickedHandler);
    }
    // 绑定功能按钮点击事件
    QList<QPushButton*> funBtns = m_view->getFunButtons();
    for(QPushButton* btn : funBtns) {
        connect(btn, &QPushButton::clicked, this, &Controller::FunButtonClickedHandler);
    }
    // 绑定更新视频流信号槽
    connect(m_model, &Model::frameReady, this, &Controller::onFrameReady);

    // 绑定矩形框确认信号
    connect(m_view, &View::rectangleConfirmed, this, &Controller::onRectangleConfirmed);

    // 绑定归一化矩形框信号
    connect(m_view, &View::normalizedRectangleConfirmed, this, &Controller::onNormalizedRectangleConfirmed);
}

Controller::~Controller()
{
    m_model->stopStream();
    m_model->wait();
}

void Controller::setTcpServer(Tcpserver* tcpServer)
{
    tcpWin = tcpServer;
}

// 添加摄像头按钮点击处理函数
void Controller::onAddCameraClicked()
{
    // 创建输入对话框用于输入RTSP地址
    QInputDialog inputDialog(m_view);
    inputDialog.setWindowTitle("输入RTSP地址");
    inputDialog.setLabelText("RTSP URL:");
    inputDialog.setTextValue("rtsp://192.168.1.130/live/0"); // 默认地址

    // 设置输入框宽度
    QLineEdit *lineEdit = inputDialog.findChild<QLineEdit *>();
    if (lineEdit)
        lineEdit->setMinimumWidth(500);

    // 显示对话框并获取用户输入
    int ret = inputDialog.exec();
    QString url = inputDialog.textValue();

    // 检查用户是否点击了确定且输入不为空
    if (ret != QDialog::Accepted || url.isEmpty())
    {
        // 未输入地址时弹出警告
        QMessageBox::warning(m_view, "错误", "未输入RTSP地址");
        return;
    }

    // 启动视频流
    m_model->startStream(url);
}

void Controller::onFrameReady(const QImage& img)
{
    if (!img.isNull())
    {
        m_lastImage = img; // 保存最近一帧
        m_view->getVideoLabel()->setPixmap(QPixmap::fromImage(img).scaled(m_view->getVideoLabel()->size(), Qt::KeepAspectRatio));
    }
}

void Controller::saveImage()
{
    if (m_lastImage.isNull()) {
        QMessageBox::warning(m_view, "提示", "当前没有可保存的图像！");
        return;
    }
    // 确保picture文件夹存在（使用源码路径）
    QString sourcePath = QString(__FILE__).section('/', 0, -2); // 获取源码目录路径
    QDir dir(sourcePath + "/picture/save-picture");
    if (!dir.exists()) dir.mkpath(".");
    // 生成文件名
    QString fileName = dir.filePath(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz") + ".jpg");
    if (m_lastImage.save(fileName)) {
        QMessageBox::information(m_view, "截图成功", "图片已保存到: " + fileName);
    } else {
        QMessageBox::warning(m_view, "保存失败", "图片保存失败！");
    }
}

void Controller::ButtonClickedHandler()
{
    QPushButton* clickedButton = qobject_cast<QPushButton*>(sender());  // 获取信号源
    if (!clickedButton)
        return;

    // 或通过动态属性区分
    int id = clickedButton->property("ButtonID").toInt();
    switch (id)
    {
    case 0:
        qDebug() << "添加摄像头";
        Controller::onAddCameraClicked();
        tcpWin->Tcp_sent_info(DEVICE_CAMERA, RTSP_ENABLE, 1);
        break;

    case 1:
        qDebug() << "暂停/恢复";
        if (!m_paused) {
            m_model->pauseStream();
            m_paused = true;
            tcpWin->Tcp_sent_info(DEVICE_CAMERA, RTSP_ENABLE, 0);
            qDebug() << "已暂停";
        } else {
            m_model->resumeStream();
            m_paused = false;
            tcpWin->Tcp_sent_info(DEVICE_CAMERA, RTSP_ENABLE, 1);
            qDebug() << "已恢复";
        }
        break;

    case 2:
        qDebug() << "截图";
        saveImage(); // 调用截图保存
        break;

    case 3:
        qDebug() << "相册";
        {
            Picture* album = new Picture(); // 创建相册窗口对象
            album->setAttribute(Qt::WA_DeleteOnClose); // 关闭时自动释放
            album->show();
        }
        break;

    case 4:
        qDebug() << "绘框";
        {
            // 切换绘制功能的启用状态
            bool currentState = m_view->isDrawingEnabled();
            m_view->enableDrawing(!currentState);

            if (!currentState) {
                // 启用绘制功能
                QMessageBox::information(m_view, "绘框功能",
                    "绘框功能已启动！\n\n"
                    "使用方法：\n"
                    "1. 在视频区域按住鼠标左键\n"
                    "2. 拖动鼠标绘制矩形框\n"
                    "3. 释放鼠标完成绘制\n\n"
                    "提示：\n"
                    "• 绘制过程中会显示红色虚线边框\n"
                    "• 完成绘制后会显示绿色实线边框\n"
                    "• 鼠标在视频区域会变为十字光标\n"
                    "• 矩形框坐标将自动保存\n\n"
                    "再次点击绘框按钮可关闭绘制功能");
            } else {
                // 禁用绘制功能
                m_view->clearRectangle();
                QMessageBox::information(m_view, "绘框功能", "绘框功能已关闭！");
            }

            // 获取当前矩形框信息（如果有的话）
            RectangleBox currentRect = m_view->getCurrentRectangle();
            if (currentRect.width > 0 && currentRect.height > 0) {
                qDebug() << "当前矩形框坐标:"
                         << "x=" << currentRect.x
                         << "y=" << currentRect.y
                         << "width=" << currentRect.width
                         << "height=" << currentRect.height;
            }
        }
        break;

    case 5:
        qDebug() << "TCP";
        {
            if (tcpWin) {
                // 显示原有的TCP服务器窗口
                tcpWin->show();
                tcpWin->raise();
                tcpWin->activateWindow();
            } else {
                QMessageBox::warning(m_view, "TCP服务器状态",
                    "TCP服务器未启动！\n"
                    "请检查系统配置。");
            }
        }
        break;
    }
}

void Controller::ServoButtonClickedHandler()
{
    QPushButton* clickedButton = qobject_cast<QPushButton*>(sender());
    if (!clickedButton)
        return;
    int id = clickedButton->property("ButtonID").toInt();

    // 获取当前步进值
    int stepValue = m_view->getStepValue();

    switch (id)
    {
    case 0:
        qDebug() << "云台 上，步进值:" << stepValue;
        tcpWin->Tcp_sent_info(DEVICE_SERVO, SERVO_UP, stepValue);
        break;
    case 1:
        qDebug() << "云台 下，步进值:" << stepValue;
        tcpWin->Tcp_sent_info(DEVICE_SERVO, SERVO_DOWN, stepValue);
        break;
    case 2:
        qDebug() << "云台 左，步进值:" << stepValue;
        tcpWin->Tcp_sent_info(DEVICE_SERVO, SERVO_LEFT, stepValue);
        break;
    case 3:
        qDebug() << "云台 右，步进值:" << stepValue;
        tcpWin->Tcp_sent_info(DEVICE_SERVO, SERVO_RIGHT, stepValue);
        break;
    case 4:
        qDebug() << "云台 复位，步进值:" << stepValue;
        tcpWin->Tcp_sent_info(DEVICE_SERVO, SERVO_RESET, stepValue);
        break;
    }

}

void Controller::FunButtonClickedHandler()
{
    QPushButton* clickedButton = qobject_cast<QPushButton*>(sender());
    if (!clickedButton)
        return;

    int id = clickedButton->property("ButtonID").toInt();
    bool isChecked = clickedButton->isChecked();

    // 更新按钮依赖关系
    updateButtonDependencies(id, isChecked);

    switch (id)
    {
    case 0: // AI功能
        qDebug() << "AI功能按钮被点击";
        if (isChecked) {
            qDebug() << "AI功能已开启";
            tcpWin->Tcp_sent_info(DEVICE_CAMERA, CAMERA_AI_ENABLE, 1);
            // 这里可以添加AI功能开启的逻辑
            QMessageBox::information(m_view, "AI功能", "AI功能已开启！");
        } else {
            qDebug() << "AI功能已关闭";
            tcpWin->Tcp_sent_info(DEVICE_CAMERA, CAMERA_AI_ENABLE, 0);
            // 这里可以添加AI功能关闭的逻辑
            QMessageBox::information(m_view, "AI功能", "AI功能已关闭！");
        }
        break;

    case 1: // 区域识别
        qDebug() << "区域识别按钮被点击";
        if (isChecked) {
            qDebug() << "区域识别已开启";
            tcpWin->Tcp_sent_info(DEVICE_CAMERA, CAMERA_REGION_ENABLE, 1);
            // 检查是否有已绘制的矩形框
            RectangleBox currentRect = m_view->getCurrentRectangle();
            if (currentRect.width > 0 && currentRect.height > 0) {
                // 有矩形框，通过TCP发送矩形框信息
                if (tcpWin) {
                    tcpWin->Tcp_sent_rect(currentRect.x, currentRect.y,
                                        currentRect.width, currentRect.height);
                    QMessageBox::information(m_view, "区域识别",
                        QString("区域识别功能已开启！\n已发送矩形框信息：\n"
                               "坐标: (%1, %2)\n尺寸: %3×%4")
                        .arg(currentRect.x)
                        .arg(currentRect.y)
                        .arg(currentRect.width)
                        .arg(currentRect.height));
                } else {
                    QMessageBox::warning(m_view, "区域识别",
                        "区域识别功能已开启！\n但TCP服务器未启动，无法发送矩形框信息。");
                }
            } else {
                // 没有矩形框，提示用户先绘制
                QMessageBox::information(m_view, "区域识别",
                    "区域识别功能已开启！\n请先绘制识别区域。");
            }
        } else {
            qDebug() << "区域识别已关闭";
            tcpWin->Tcp_sent_info(DEVICE_CAMERA, CAMERA_REGION_ENABLE, 0);
            QMessageBox::information(m_view, "区域识别", "区域识别功能已关闭！");
        }
        break;

    case 2: // 对象识别
        qDebug() << "对象识别按钮被点击";
        if (isChecked) {
            qDebug() << "对象识别已开启";
            tcpWin->Tcp_sent_info(DEVICE_CAMERA, CAMERA_OBJECT_ENABLE, 1);
            // 这里可以添加对象识别开启的逻辑
            QMessageBox::information(m_view, "对象识别", "对象识别功能已开启！");
        } else {
            qDebug() << "对象识别已关闭";
            tcpWin->Tcp_sent_info(DEVICE_CAMERA, CAMERA_OBJECT_ENABLE, 0);
            // 这里可以添加对象识别关闭的逻辑
            QMessageBox::information(m_view, "对象识别", "对象识别功能已关闭！");
        }
        break;

    case 3: // 对象列表
        qDebug() << "对象列表按钮被点击";
        {
            // 创建或显示对象检测列表窗口
            if (!m_detectList) {
                m_detectList = new DetectList();
                m_detectList->setAttribute(Qt::WA_DeleteOnClose); // 关闭时自动释放
                // 连接对象检测列表的 selectionChanged 信号到 Controller 的槽函数
                connect(m_detectList, &DetectList::selectionChanged,
                        this, &Controller::onDetectListSelectionChanged);
                // 当 DetectList 窗口被销毁时，将 m_detectList 指针置为 nullptr
                connect(m_detectList, &QObject::destroyed,
                        [this]() { m_detectList = nullptr; });
            }

            // 设置当前选中的对象
            m_detectList->setSelectedObjects(m_selectedObjectIds);

            // 显示窗口
            m_detectList->show();
            m_detectList->raise();
            m_detectList->activateWindow();
        }
        break;

    default:
        qDebug() << "未知功能按钮ID:" << id;
        break;
    }
}

void Controller::onDetectListSelectionChanged(const QSet<int>& selectedIds)
{
    m_selectedObjectIds = selectedIds;

    qDebug() << "对象检测列表选择已更新. Count:" << selectedIds.size();

    // 获取对象名称列表
    QStringList objectNames = DetectList::getObjectNames();
    QStringList selectedNames;

    // 根据选中的ID获取对应的对象名称
    for (int id : selectedIds) {
        if (id >= 0 && id < objectNames.size()) {
            selectedNames.append(objectNames[id]);
        }
    }

    qDebug() << "选中的对象名称:" << selectedNames;

    // 通过TCP发送对象列表信息
    if (tcpWin) {
        tcpWin->Tcp_sent_list(selectedIds);
        QMessageBox::information(m_view, "对象检测设置",
            QString("已选择 %1 个对象进行检测：\n\n%2\n\n对象列表已通过TCP发送！")
            .arg(selectedIds.size())
            .arg(selectedNames.isEmpty() ? "未选择任何对象" : selectedNames.join(", ")));
    } else {
        QMessageBox::warning(m_view, "对象检测设置",
            QString("已选择 %1 个对象进行检测：\n\n%2\n\n但TCP服务器未启动，无法发送对象列表。")
            .arg(selectedIds.size())
            .arg(selectedNames.isEmpty() ? "未选择任何对象" : selectedNames.join(", ")));
    }
}

void Controller::onRectangleConfirmed(const RectangleBox& rect)
{
    qDebug() << "Controller接收到矩形框确认信号:" << rect.x << rect.y << rect.width << rect.height;

    // 检查区域识别是否已开启
    QList<QPushButton*> funButtons = m_view->getFunButtons();
    if (funButtons.size() > 1 && funButtons[1]->isChecked()) {
        // 区域识别已开启，发送矩形框信息
        if (tcpWin) {
            tcpWin->Tcp_sent_rect(rect.x, rect.y, rect.width, rect.height);
            QMessageBox::information(m_view, "区域识别",
                QString("矩形框已确认并发送！\n"
                       "坐标: (%1, %2)\n尺寸: %3×%4")
                .arg(rect.x)
                .arg(rect.y)
                .arg(rect.width)
                .arg(rect.height));
        } else {
            QMessageBox::warning(m_view, "区域识别",
                "矩形框已确认！\n但TCP服务器未启动，无法发送矩形框信息。");
        }
    }
}

void Controller::onNormalizedRectangleConfirmed(const NormalizedRectangleBox& normRect, const RectangleBox& absRect)
{
    qDebug() << "Controller接收到归一化矩形框信号:" << normRect.x << normRect.y << normRect.width << normRect.height;
    qDebug() << "Controller接收到原始矩形框信号:" << absRect.x << absRect.y << absRect.width << absRect.height;

    // 检查区域识别是否已开启
    QList<QPushButton*> funButtons = m_view->getFunButtons();
    if (funButtons.size() > 1 && funButtons[1]->isChecked()) {
        // 区域识别已开启，发送归一化矩形框信息
        if (tcpWin) {
            tcpWin->Tcp_sent_rect(normRect.x, normRect.y, normRect.width, normRect.height);
            QMessageBox::information(m_view, "区域识别",
                QString("归一化矩形框已确认并发送！\n"
                       "归一化坐标: (%.4f, %.4f)\n归一化尺寸: %.4f×%.4f")
                    .arg(normRect.x, 0, 'f', 4)
                    .arg(normRect.y, 0, 'f', 4)
                    .arg(normRect.width, 0, 'f', 4)
                    .arg(normRect.height, 0, 'f', 4));
        } else {
            QMessageBox::warning(m_view, "区域识别",
                "归一化矩形框已确认！\n但TCP服务器未启动，无法发送矩形框信息。");
        }
    }
}

void Controller::updateButtonDependencies(int clickedButtonId, bool isChecked)
{
    QList<QPushButton*> funButtons = m_view->getFunButtons();
    if (funButtons.size() < 3) return;

    switch (clickedButtonId) {
    case 0: // AI功能按钮
        if (!isChecked) {
            // AI功能关闭时，同时关闭区域识别
            QPushButton* areaRecognitionBtn = funButtons[1];
            QPushButton* objectRecognitionBtn = funButtons[2];
            if (areaRecognitionBtn->isChecked() || objectRecognitionBtn->isChecked()) {
                areaRecognitionBtn->setChecked(false);
                objectRecognitionBtn->setChecked(false);
                qDebug() << "AI功能已关闭，同时关闭区域识别与对象识别";
                tcpWin->Tcp_sent_info(DEVICE_CAMERA, CAMERA_REGION_ENABLE, 0);
                tcpWin->Tcp_sent_info(DEVICE_CAMERA, CAMERA_OBJECT_ENABLE, 0);
                QMessageBox::information(m_view, "区域识别", "AI功能已关闭，区域识别与对象识别功能也已关闭！");
            }
        }
        break;

    case 1: // 区域识别按钮
        if (isChecked) {
            // 区域识别开启时，确保AI功能也开启
            QPushButton* aiBtn = funButtons[0];
            if (!aiBtn->isChecked()) {
                aiBtn->setChecked(true);
                qDebug() << "区域识别需要AI功能，自动开启AI功能";
                tcpWin->Tcp_sent_info(DEVICE_CAMERA, CAMERA_AI_ENABLE, 1);
                QMessageBox::information(m_view, "AI功能", "区域识别功能需要AI功能支持，已自动开启AI功能！");
            }
        }
        break;

    case 2: // 对象识别按钮
        if (isChecked) {
            // 对象识别开启时，确保AI功能也开启
            QPushButton* aiBtn = funButtons[0];
            if (!aiBtn->isChecked()) {
                aiBtn->setChecked(true);
                qDebug() << "对象识别需要AI功能，自动开启AI功能";
                tcpWin->Tcp_sent_info(DEVICE_CAMERA, CAMERA_AI_ENABLE, 1);
                QMessageBox::information(m_view, "AI功能", "对象识别功能需要AI功能支持，已自动开启AI功能！");
            }
        }
        break;
    }
}
