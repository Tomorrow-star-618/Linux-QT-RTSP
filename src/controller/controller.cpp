#include "controller.h"
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>
#include <QRegExp>
#include "Picture.h"
#include "Tcpserver.h" // Added for Tcpserver
#include "plan.h"      // Added for Plan and PlanData
#include "common.h"
#include "../view/AddCameraDialog.h" // 添加摄像头对话框

Controller::Controller(Model* model, View* view, QObject* parent)
    : QObject(parent), m_model(model), m_view(view), m_nextStreamId(1)
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
    connect(m_model, &Model::streamDisconnected, this, [this](const QString& url) {
        m_view->addEventMessage("warning", QString("视频流断开: %1").arg(url));
    });
    connect(m_model, &Model::streamReconnecting, this, [this](const QString& url) {
        m_view->addEventMessage("info", QString("正在尝试重连: %1").arg(url));
    });

    // 绑定矩形框确认信号
    connect(m_view, &View::rectangleConfirmed, this, &Controller::onRectangleConfirmed);

    // 绑定归一化矩形框信号
    connect(m_view, &View::normalizedRectangleConfirmed, this, &Controller::onNormalizedRectangleConfirmed);

    // 绑定多路视频流信号
    connect(m_view, &View::layoutModeChanged, this, &Controller::onLayoutModeChanged);
    connect(m_view, &View::streamSelected, this, &Controller::onStreamSelected);
    connect(m_view, &View::streamPauseRequested, this, &Controller::onStreamPauseRequested);
    connect(m_view, &View::streamScreenshotRequested, this, &Controller::onStreamScreenshotRequested);
    connect(m_view, &View::addCameraWithIdRequested, this, &Controller::onAddCameraWithIdRequested);

    // 如果稍后设置tcpWin，也会在setTcpServer中再连接
    if (tcpWin) {
        connect(tcpWin, &Tcpserver::tcpClientConnected, this, &Controller::onTcpClientConnected);
        connect(tcpWin, &Tcpserver::detectionDataReceived, this, &Controller::onDetectionDataReceived);
    }
}

Controller::~Controller()
{
    // 清理所有多路视频流
    clearAllStreams();
    
    // 清理主视频流
    m_model->stopStream();
    m_model->wait();
}

void Controller::setTcpServer(Tcpserver* tcpServer)
{
    tcpWin = tcpServer;
    if (tcpWin) {
        connect(tcpWin, &Tcpserver::tcpClientConnected, this, &Controller::onTcpClientConnected);
        connect(tcpWin, &Tcpserver::detectionDataReceived, this, &Controller::onDetectionDataReceived);
    }
}

// 添加摄像头按钮点击处理函数
void Controller::onAddCameraClicked()
{
    // 获取可用的摄像头位置ID (1-16)
    QList<int> availableIds = m_view->getAvailableCameraIds();
    if (availableIds.isEmpty()) {
        QMessageBox::warning(m_view, "错误", "所有摄像头位置（1-16）已被占用！\n请先删除已有的摄像头。");
        m_view->addEventMessage("warning", "所有摄像头位置已被占用");
        return;
    }
    
    // 创建并显示自定义添加摄像头对话框
    AddCameraDialog dialog(availableIds, m_view);
    
    if (dialog.exec() == QDialog::Accepted) {
        // 获取用户输入的信息
        int cameraId = dialog.getSelectedCameraId();
        QString url = dialog.getRtspUrl();
        QString name = dialog.getCameraName();
        
        // 添加视频流
        addVideoStream(url, name, cameraId);
        
        m_view->addEventMessage("success", QString("正在添加摄像头 %1: %2").arg(cameraId).arg(name));
    } else {
        // 用户取消了添加操作
        m_view->addEventMessage("info", "取消添加摄像头");
    }
}

void Controller::onFrameReady(const QImage& img)
{
    if (!img.isNull())
    {
        m_lastImage = img; // 保存最近一帧图像
        
        // 主视频流现在主要用于绘框功能
        // 只在需要时更新videoLabel（绘框模式或旧代码兼容）
        VideoLabel* videoLabel = m_view->getVideoLabel();
        if (videoLabel && videoLabel->isVisible()) {
            videoLabel->setPixmap(QPixmap::fromImage(img).scaled(videoLabel->size(), Qt::KeepAspectRatio));
        }
    }
}

void Controller::saveImage()
{
    if (m_lastImage.isNull()) {
        QMessageBox::warning(m_view, "提示", "当前没有可保存的图像！");
        m_view->addEventMessage("warning", "当前没有可保存的图像！");
        return;
    }
    // 确保picture文件夹存在（使用项目根目录路径）
    // __FILE__ 在 src/controller/controller.cpp，需要回退到项目根目录
    QString sourcePath = QString(__FILE__).section('/', 0, -4); // 回退到项目根目录
    QDir dir(sourcePath + "/picture/save-picture");
    if (!dir.exists()) dir.mkpath(".");
    // 生成文件名
    QString fileName = dir.filePath(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz") + ".jpg");
    if (m_lastImage.save(fileName)) {
        QMessageBox::information(m_view, "截图成功", "图片已保存到: " + fileName);
        m_view->addEventMessage("success", "截图成功，图片已保存到: " + fileName);
    } else {
        QMessageBox::critical(m_view, "保存失败", "图片保存失败！");
        m_view->addEventMessage("error", "图片保存失败！");
    }
}

void Controller::saveAlarmImage(int cameraId, const QString& detectionInfo)
{
    // 检查报警保存功能是否开启
    if (!m_alarmSaveEnabled) {
        qDebug() << "报警保存功能未开启，跳过保存图片";
        return;
    }
    
    // 根据摄像头ID获取对应的流图像
    QImage imageToSave;
    
    if (cameraId > 0) {
        // 尝试从View的VideoLabel直接获取该摄像头的当前图像
        imageToSave = m_view->getCurrentFrameForCamera(cameraId);
        if (!imageToSave.isNull()) {
            qDebug() << "成功获取摄像头" << cameraId << "的当前帧图像";
        } else {
            qDebug() << "警告：无法获取摄像头" << cameraId << "的图像";
        }
    }
    
    // 如果未指定摄像头或获取失败，使用主流图像（m_lastImage）作为备用
    if (imageToSave.isNull()) {
        if (!m_lastImage.isNull()) {
            imageToSave = m_lastImage;
            cameraId = 0; // 标记为主流
            qDebug() << "警告：无法获取摄像头" << cameraId << "的图像，使用主流图像";
        } else {
            qDebug() << "错误：当前没有可保存的图像！";
            m_view->addEventMessage("warning", "检测到目标但当前没有可保存的图像！");
            return;
        }
    }
    
    // 确保报警图片目录存在（使用项目根目录路径）
    // __FILE__ 在 src/controller/controller.cpp，需要回退到项目根目录
    QString sourcePath = QString(__FILE__).section('/', 0, -4); // 回退到项目根目录
    QDir dir(sourcePath + "/picture/alarm-picture");
    if (!dir.exists()) {
        dir.mkpath("."); // 创建目录
    }
    
    // 生成报警图片文件名，格式：alarm_cam{摄像头ID}_{时间精确到秒}_{毫秒}.jpg
    // 例如：alarm_cam1_20251013_155943_515.jpg
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
    QString fileName;
    if (cameraId > 0) {
        fileName = dir.filePath(QString("alarm_cam%1_%2.jpg").arg(cameraId).arg(timestamp));
    } else {
        // cameraId为0或-1时，使用main作为标识
        fileName = dir.filePath(QString("alarm_main_%1.jpg").arg(timestamp));
    }
    
    // 保存图像
    if (imageToSave.save(fileName)) {
        QString successMsg = QString("摄像头%1检测到目标，报警图片已保存: %2").arg(cameraId).arg(fileName);
        qDebug() << successMsg;
        m_view->addEventMessage("alarm", successMsg);
    } else {
        qDebug() << "错误：报警图片保存失败！";
        m_view->addEventMessage("error", "报警图片保存失败！");
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
        if (tcpWin && tcpWin->hasConnectedClients()) {
            int targetCameraId = tcpWin->getCurrentCameraId();
            tcpWin->Tcp_sent_info(targetCameraId, DEVICE_CAMERA, RTSP_ENABLE, 1);
        } else {
            m_view->addEventMessage("warning", "请先连接TCP服务");
        }
        break;

    case 1:
        qDebug() << "暂停/恢复";
        {
            // 获取当前选中的摄像头ID
            int currentCameraId = tcpWin ? tcpWin->getCurrentCameraId() : -1;
            
            if (currentCameraId <= 0) {
                m_view->addEventMessage("warning", "请先选中一个摄像头");
                qDebug() << "警告：未选中摄像头，无法执行暂停/恢复操作";
                break;
            }
            
            // 根据摄像头ID获取对应的流ID
            int streamId = m_view->getStreamIdForCamera(currentCameraId);
            if (streamId == -1) {
                m_view->addEventMessage("warning", QString("摄像头%1没有对应的视频流").arg(currentCameraId));
                qDebug() << "警告：摄像头" << currentCameraId << "没有对应的视频流";
                break;
            }
            
            // 获取对应的Model
            if (!m_streamModels.contains(streamId)) {
                m_view->addEventMessage("warning", QString("视频流%1不存在").arg(streamId));
                qDebug() << "警告：视频流" << streamId << "不存在";
                break;
            }
            
            Model* model = m_streamModels.value(streamId);
            
            // 切换暂停/恢复状态
            if (model->isPaused()) {
                model->resumeStream();
                m_view->addEventMessage("success", QString("已恢复摄像头%1的视频流").arg(currentCameraId));
                qDebug() << "已恢复摄像头" << currentCameraId << "的视频流";
                
                if (tcpWin && tcpWin->hasConnectedClients()) {
                    tcpWin->Tcp_sent_info(currentCameraId, DEVICE_CAMERA, RTSP_ENABLE, 1);
                }
            } else {
                model->pauseStream();
                m_view->addEventMessage("info", QString("已暂停摄像头%1的视频流").arg(currentCameraId));
                qDebug() << "已暂停摄像头" << currentCameraId << "的视频流";
                
                if (tcpWin && tcpWin->hasConnectedClients()) {
                    tcpWin->Tcp_sent_info(currentCameraId, DEVICE_CAMERA, RTSP_ENABLE, 0);
                }
            }
        }
        break;

    case 2:
        qDebug() << "截图";
        {
            // 获取当前选中的摄像头ID
            int currentCameraId = tcpWin ? tcpWin->getCurrentCameraId() : -1;
            
            if (currentCameraId <= 0) {
                m_view->addEventMessage("warning", "请先选中一个摄像头");
                qDebug() << "警告：未选中摄像头，无法执行截图操作";
                break;
            }
            
            // 根据摄像头ID获取对应的流ID
            int streamId = m_view->getStreamIdForCamera(currentCameraId);
            if (streamId == -1) {
                m_view->addEventMessage("warning", QString("摄像头%1没有对应的视频流").arg(currentCameraId));
                qDebug() << "警告：摄像头" << currentCameraId << "没有对应的视频流";
                break;
            }
            
            // 调用针对指定流的截图功能
            onStreamScreenshotRequested(streamId);
        }
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
            // 如果绘框功能已经启用，则关闭并返回多路显示
            if (m_view->isDrawingEnabled()) {
                // 禁用绘制功能
                m_view->enableDrawing(false);
                m_view->clearRectangle();
                
                // 切换回4路显示模式（默认布局）
                m_view->switchToLayoutMode(4);
                
                QMessageBox::information(m_view, "绘框功能", "绘框功能已关闭！\n已切换回多路显示。");
                m_view->addEventMessage("info", "绘框功能已关闭，已切换回多路显示");
                break;
            }
            
            // 检查是否有选中的摄像头
            int selectedStreamId = m_view->getSelectedStreamId();
            int targetCameraId = -1;
            
            if (selectedStreamId == -1) {
                // 没有选中的摄像头，提示用户选择
                QList<int> usedCameraIds = m_view->getUsedCameraIds();
                if (usedCameraIds.isEmpty()) {
                    QMessageBox::warning(m_view, "绘框功能", 
                        "没有可用的摄像头！\n请先添加摄像头。");
                    m_view->addEventMessage("warning", "绘框失败：没有可用的摄像头");
                    break;
                }
                
                // 弹窗让用户选择摄像头ID
                QStringList cameraIdList;
                for (int id : usedCameraIds) {
                    QString name = m_view->getStreamName(m_view->getStreamIdForCamera(id));
                    cameraIdList << QString("摄像头 %1 - %2").arg(id).arg(name);
                }
                
                bool ok;
                QString selectedItem = QInputDialog::getItem(m_view, "选择摄像头",
                    "请选择要绘框的摄像头：", cameraIdList, 0, false, &ok);
                
                if (!ok || selectedItem.isEmpty()) {
                    m_view->addEventMessage("info", "绘框操作已取消");
                    break;
                }
                
                // 提取摄像头ID
                int idx = cameraIdList.indexOf(selectedItem);
                if (idx >= 0 && idx < usedCameraIds.size()) {
                    targetCameraId = usedCameraIds[idx];
                    selectedStreamId = m_view->getStreamIdForCamera(targetCameraId);
                } else {
                    QMessageBox::warning(m_view, "绘框功能", "选择的摄像头无效！");
                    break;
                }
            } else {
                // 已有选中的摄像头
                targetCameraId = m_view->getCameraIdForStream(selectedStreamId);
            }
            
            // 确保有有效的摄像头ID
            if (targetCameraId <= 0 || selectedStreamId == -1) {
                QMessageBox::warning(m_view, "绘框功能", "无法获取有效的摄像头信息！");
                m_view->addEventMessage("warning", "绘框失败：摄像头信息无效");
                break;
            }
            
            // 切换到单路显示模式
            m_view->switchToFullScreen(selectedStreamId);
            QString cameraName = m_view->getStreamName(selectedStreamId);
            m_view->addEventMessage("info", QString("绘框模式：切换到单路显示 - 摄像头%1 %2")
                .arg(targetCameraId).arg(cameraName));
            
            // 启用绘制功能
            m_view->enableDrawing(true);
            
            QMessageBox::information(m_view, "绘框功能",
                QString("绘框功能已启动！\n目标摄像头：%1 - %2\n\n"
                    "使用方法：\n"
                    "1. 在视频区域按住鼠标左键\n"
                    "2. 拖动鼠标绘制矩形框\n"
                    "3. 释放鼠标完成绘制\n\n"
                    "提示：\n"
                    "• 绘制过程中会显示红色虚线边框\n"
                    "• 完成绘制后会显示绿色实线边框\n"
                    "• 鼠标在视频区域会变为十字光标\n"
                    "• 矩形框数据将通过TCP发送\n\n"
                    "再次点击绘框按钮可关闭绘制功能")
                .arg(targetCameraId).arg(cameraName));
            m_view->addEventMessage("info", QString("绘框功能已启动：摄像头%1").arg(targetCameraId));
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
                m_view->addEventMessage("warning", "TCP服务器未启动！请检查系统配置");
            }
        }
        break;

    default:
        qDebug() << "未知功能按钮ID:" << id;
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

    // 判断是否有TCP连接
    if (!(tcpWin && tcpWin->hasConnectedClients())) {
        m_view->addEventMessage("warning", "请先连接TCP服务");
        return;
    }

    int targetCameraId = tcpWin->getCurrentCameraId();
    
    // 调试输出：检查当前摄像头ID
    qDebug() << "ServoButtonClickedHandler: targetCameraId =" << targetCameraId;
    
    // 检查摄像头ID是否有效
    if (targetCameraId <= 0) {
        m_view->addEventMessage("warning", "请先选中一个摄像头，然后再进行云台控制！");
        qDebug() << "警告：未选中摄像头，targetCameraId =" << targetCameraId;
        return;
    }
    
    switch (id)
    {
    case 0:
        qDebug() << "云台 上，步进值:" << stepValue;
        tcpWin->Tcp_sent_info(targetCameraId, DEVICE_SERVO, SERVO_UP, stepValue);
        m_view->addEventMessage("info", QString("云台上转，步进值: %1").arg(stepValue));
        break;
    case 1:
        qDebug() << "云台 下，步进值:" << stepValue;
        tcpWin->Tcp_sent_info(targetCameraId, DEVICE_SERVO, SERVO_DOWN, stepValue);
        m_view->addEventMessage("info", QString("云台下转，步进值: %1").arg(stepValue));
        break;
    case 2:
        qDebug() << "云台 左，步进值:" << stepValue;
        tcpWin->Tcp_sent_info(targetCameraId, DEVICE_SERVO, SERVO_LEFT, stepValue);
        m_view->addEventMessage("info", QString("云台左转，步进值: %1").arg(stepValue));
        break;
    case 3:
        qDebug() << "云台 右，步进值:" << stepValue;
        tcpWin->Tcp_sent_info(targetCameraId, DEVICE_SERVO, SERVO_RIGHT, stepValue);
        m_view->addEventMessage("info", QString("云台右转，步进值: %1").arg(stepValue));
        break;
    case 4:
        qDebug() << "云台 复位，步进值:" << stepValue;
        tcpWin->Tcp_sent_info(targetCameraId, DEVICE_SERVO, SERVO_RESET, stepValue);
        m_view->addEventMessage("info", QString("云台复位"));
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

    switch (id)
    {
    case 0: // AI功能
        qDebug() << "AI功能按钮被点击";
        if (!(tcpWin && tcpWin->hasConnectedClients())) {
            m_view->addEventMessage("warning", "请先连接TCP服务");
            clickedButton->setChecked(!isChecked); // 回滚状态
            return;
        }
        {
            int targetCameraId = tcpWin->getCurrentCameraId();
            qDebug() << "AI功能: targetCameraId =" << targetCameraId;
            if (isChecked) {
                qDebug() << "AI功能已开启";
                tcpWin->Tcp_sent_info(targetCameraId, DEVICE_CAMERA, CAMERA_AI_ENABLE, 1);
                QMessageBox::information(m_view, "AI功能", "AI功能已开启！");
                m_view->addEventMessage("info", "AI功能已开启！");
            } else {
                qDebug() << "AI功能已关闭";
                tcpWin->Tcp_sent_info(targetCameraId, DEVICE_CAMERA, CAMERA_AI_ENABLE, 0);
                QMessageBox::information(m_view, "AI功能", "AI功能已关闭！");
                m_view->addEventMessage("info", "AI功能已关闭！");
            }
        }
        break;

    case 1: // 区域识别
        qDebug() << "区域识别按钮被点击";
        if (!(tcpWin && tcpWin->hasConnectedClients())) {
            m_view->addEventMessage("warning", "请先连接TCP服务");
            clickedButton->setChecked(!isChecked);
            return;
        }
        {
            int targetCameraId = tcpWin->getCurrentCameraId();
            if (isChecked) {
                qDebug() << "区域识别已开启";
                tcpWin->Tcp_sent_info(targetCameraId, DEVICE_CAMERA, CAMERA_REGION_ENABLE, 1);
            // 检查是否有已绘制的矩形框
            RectangleBox currentRect = m_view->getCurrentRectangle();
            if (currentRect.width > 0 && currentRect.height > 0) {
                // 有矩形框，通过TCP发送矩形框信息
                int targetCameraId = tcpWin->getCurrentCameraId();
                tcpWin->Tcp_sent_rect(targetCameraId, currentRect.x, currentRect.y,
                                        currentRect.width, currentRect.height);
                QMessageBox::information(m_view, "区域识别",
                    QString("区域识别功能已开启！\n已发送矩形框信息：\n"
                           "坐标: (%1, %2)\n尺寸: %3×%4")
                    .arg(currentRect.x)
                    .arg(currentRect.y)
                    .arg(currentRect.width)
                    .arg(currentRect.height));
                m_view->addEventMessage("info", QString("区域识别功能已开启！已发送矩形框信息：坐标(%1,%2) 尺寸%3×%4")
                    .arg(currentRect.x).arg(currentRect.y).arg(currentRect.width).arg(currentRect.height));
            } else {
                // 没有矩形框，提示用户先绘制
                QMessageBox::information(m_view, "区域识别",
                    "区域识别功能已开启！\n请先绘制识别区域。");
                m_view->addEventMessage("info", "区域识别功能已开启！请先绘制识别区域");
            }
            } else {
                qDebug() << "区域识别已关闭";
                tcpWin->Tcp_sent_info(targetCameraId, DEVICE_CAMERA, CAMERA_REGION_ENABLE, 0);
                QMessageBox::information(m_view, "区域识别", "区域识别功能已关闭！");
                m_view->addEventMessage("info", "区域识别功能已关闭！");
            }
        }
        break;

    case 2: // 对象识别
        qDebug() << "对象识别按钮被点击";
        if (!(tcpWin && tcpWin->hasConnectedClients())) {
            m_view->addEventMessage("warning", "请先连接TCP服务");
            clickedButton->setChecked(!isChecked);
            return;
        }
        {
            int targetCameraId = tcpWin->getCurrentCameraId();
            if (isChecked) {
                qDebug() << "对象识别已开启";
                tcpWin->Tcp_sent_info(targetCameraId, DEVICE_CAMERA, CAMERA_OBJECT_ENABLE, 1);
                QMessageBox::information(m_view, "对象识别", "对象识别功能已开启！");
                m_view->addEventMessage("info", "对象识别功能已开启！");
            } else {
                qDebug() << "对象识别已关闭";
                tcpWin->Tcp_sent_info(targetCameraId, DEVICE_CAMERA, CAMERA_OBJECT_ENABLE, 0);
                QMessageBox::information(m_view, "对象识别", "对象识别功能已关闭！");
                m_view->addEventMessage("info", "对象识别功能已关闭！");
            }
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

    case 4: //方案预选
        qDebug() << "方案预选按钮被点击";
        {
            // 创建或显示方案预选窗口
            if (!m_plan) {
                m_plan = new Plan();
                m_plan->setAttribute(Qt::WA_DeleteOnClose); // 关闭时自动释放
                // 连接方案应用信号到Controller的槽函数
                connect(m_plan, &Plan::planApplied,
                        this, &Controller::onPlanApplied);
                // 当Plan窗口被销毁时，将m_plan指针置为nullptr
                connect(m_plan, &QObject::destroyed,
                        [this]() { m_plan = nullptr; });
            }

            // 显示窗口
            m_plan->show();
            m_plan->raise();
            m_plan->activateWindow();
        }
        break;

    case 5: // 报警保存
        qDebug() << "报警保存按钮被点击";
        {
            m_alarmSaveEnabled = isChecked;
            
            if (isChecked) {
                qDebug() << "报警自动保存已开启";
                QMessageBox::information(m_view, "报警保存", 
                    "报警自动保存功能已开启！\n\n"
                    "当检测到目标时，系统将自动保存报警图片到：\n"
                    "picture/alarm-picture/ 目录");
                m_view->addEventMessage("success", "报警自动保存功能已开启！");
            } else {
                qDebug() << "报警自动保存已关闭";
                QMessageBox::information(m_view, "报警保存", "报警自动保存功能已关闭！");
                m_view->addEventMessage("info", "报警自动保存功能已关闭！");
            }
        }
        break;

    default:
        qDebug() << "未知功能按钮ID:" << id;
        break;
    }

    // 更新按钮依赖关系
    updateButtonDependencies(id, isChecked);
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
    if (tcpWin && tcpWin->hasConnectedClients()) {
        int targetCameraId = tcpWin->getCurrentCameraId();
        tcpWin->Tcp_sent_list(targetCameraId, selectedIds);
        QMessageBox::information(m_view, "对象检测设置",
            QString("已选择 %1 个对象进行检测：\n\n%2\n\n对象列表已通过TCP发送！")
            .arg(selectedIds.size())
            .arg(selectedNames.isEmpty() ? "未选择任何对象" : selectedNames.join(", ")));
        m_view->addEventMessage("info", QString("已选择 %1 个对象进行检测，对象列表已通过TCP发送！")
            .arg(selectedIds.size()));
    } else {
        QMessageBox::warning(m_view, "对象检测设置",
            QString("已选择 %1 个对象进行检测：\n\n%2\n\n暂无TCP连接，无法发送对象列表。")
            .arg(selectedIds.size())
            .arg(selectedNames.isEmpty() ? "未选择任何对象" : selectedNames.join(", ")));
        m_view->addEventMessage("warning", QString("已选择 %1 个对象进行检测，但无TCP连接")
            .arg(selectedIds.size()));
    }
}

void Controller::onRectangleConfirmed(const RectangleBox& rect)
{
    qDebug() << "Controller接收到矩形框确认信号:" << rect.x << rect.y << rect.width << rect.height;

    // 获取当前绘框对应的摄像头ID
    int selectedStreamId = m_view->getSelectedStreamId();
    int targetCameraId = -1;
    QString cameraName;
    
    if (selectedStreamId != -1) {
        targetCameraId = m_view->getCameraIdForStream(selectedStreamId);
        cameraName = m_view->getStreamName(selectedStreamId);
    }
    
    // 如果绘制功能已启用，说明是通过绘框按钮触发的绘制
    if (m_view->isDrawingEnabled()) {
        // 发送矩形框数据到TCP
        if (tcpWin && tcpWin->hasConnectedClients()) {
            if (targetCameraId > 0) {
                tcpWin->Tcp_sent_rect(targetCameraId, rect.x, rect.y, rect.width, rect.height);
                QMessageBox::information(m_view, "绘框完成",
                    QString("矩形框已绘制并发送！\n"
                           "摄像头：%1 - %2\n"
                           "坐标: (%3, %4)\n尺寸: %5×%6")
                    .arg(targetCameraId).arg(cameraName)
                    .arg(rect.x).arg(rect.y)
                    .arg(rect.width).arg(rect.height));
                m_view->addEventMessage("success", QString("绘框完成并发送：摄像头%1 坐标(%2,%3) 尺寸%4×%5")
                    .arg(targetCameraId).arg(rect.x).arg(rect.y).arg(rect.width).arg(rect.height));
            } else {
                QMessageBox::warning(m_view, "绘框完成",
                    "矩形框已绘制！\n但无法确定目标摄像头，未发送数据。");
                m_view->addEventMessage("warning", "矩形框已绘制但无法确定目标摄像头");
            }
        } else {
            QMessageBox::warning(m_view, "绘框完成",
                "矩形框已绘制！\n暂无TCP连接，无法发送矩形框信息。");
            m_view->addEventMessage("warning", "矩形框已绘制但无TCP连接，未发送");
        }
        
        // 可选：绘框完成后自动关闭绘制功能
        // m_view->enableDrawing(false);
    } 
    // 检查区域识别是否已开启（保留原有的区域识别功能）
    else {
        QList<QPushButton*> funButtons = m_view->getFunButtons();
        if (funButtons.size() > 1 && funButtons[1]->isChecked()) {
            // 区域识别已开启，发送矩形框信息
            if (tcpWin && tcpWin->hasConnectedClients()) {
                if (targetCameraId <= 0) {
                    targetCameraId = tcpWin->getCurrentCameraId();
                }
                tcpWin->Tcp_sent_rect(targetCameraId, rect.x, rect.y, rect.width, rect.height);
                QMessageBox::information(m_view, "区域识别",
                    QString("矩形框已确认并发送！\n"
                           "坐标: (%1, %2)\n尺寸: %3×%4")
                    .arg(rect.x).arg(rect.y)
                    .arg(rect.width).arg(rect.height));
                m_view->addEventMessage("info", QString("矩形框已确认并发送！坐标(%1,%2) 尺寸%3×%4")
                    .arg(rect.x).arg(rect.y).arg(rect.width).arg(rect.height));
            } else {
                QMessageBox::warning(m_view, "区域识别",
                    "矩形框已确认！\n暂无TCP连接，无法发送矩形框信息。");
                m_view->addEventMessage("warning", "矩形框已确认但无TCP连接，未发送");
            }
        }
    }
}

void Controller::onNormalizedRectangleConfirmed(const NormalizedRectangleBox& normRect, const RectangleBox& absRect)
{
    qDebug() << "Controller接收到归一化矩形框信号:" << normRect.x << normRect.y << normRect.width << normRect.height;
    qDebug() << "Controller接收到原始矩形框信号:" << absRect.x << absRect.y << absRect.width << absRect.height;

    // 获取当前绘框对应的摄像头ID
    int selectedStreamId = m_view->getSelectedStreamId();
    int targetCameraId = -1;
    QString cameraName;
    
    if (selectedStreamId != -1) {
        targetCameraId = m_view->getCameraIdForStream(selectedStreamId);
        cameraName = m_view->getStreamName(selectedStreamId);
    }
    
    // 如果绘制功能已启用，说明是通过绘框按钮触发的绘制
    if (m_view->isDrawingEnabled()) {
        // 发送归一化矩形框数据到TCP
        if (tcpWin && tcpWin->hasConnectedClients()) {
            if (targetCameraId > 0) {
                tcpWin->Tcp_sent_rect(targetCameraId, normRect.x, normRect.y, normRect.width, normRect.height);
                QString msg = QString("归一化矩形框已绘制并发送！\n"
                       "摄像头：%1 - %2\n"
                       "归一化坐标: (%.4f, %.4f)\n归一化尺寸: %.4f×%.4f")
                    .arg(targetCameraId).arg(cameraName)
                    .arg(normRect.x, 0, 'f', 4)
                    .arg(normRect.y, 0, 'f', 4)
                    .arg(normRect.width, 0, 'f', 4)
                    .arg(normRect.height, 0, 'f', 4);
                // 不重复弹窗，因为onRectangleConfirmed已经弹过
                // QMessageBox::information(m_view, "绘框完成", msg);
                m_view->addEventMessage("success", QString("归一化矩形框已发送：摄像头%1 坐标(%.4f,%.4f) 尺寸%.4f×%.4f")
                    .arg(targetCameraId).arg(normRect.x, 0, 'f', 4).arg(normRect.y, 0, 'f', 4)
                    .arg(normRect.width, 0, 'f', 4).arg(normRect.height, 0, 'f', 4));
            }
        }
    }
    // 检查区域识别是否已开启（保留原有的区域识别功能）
    else {
        QList<QPushButton*> funButtons = m_view->getFunButtons();
        if (funButtons.size() > 1 && funButtons[1]->isChecked()) {
            // 区域识别已开启，发送归一化矩形框信息
            if (tcpWin && tcpWin->hasConnectedClients()) {
                if (targetCameraId <= 0) {
                    targetCameraId = tcpWin->getCurrentCameraId();
                }
                tcpWin->Tcp_sent_rect(targetCameraId, normRect.x, normRect.y, normRect.width, normRect.height);
                QMessageBox::information(m_view, "区域识别",
                    QString("归一化矩形框已确认并发送！\n"
                           "归一化坐标: (%.4f, %.4f)\n归一化尺寸: %.4f×%.4f")
                        .arg(normRect.x, 0, 'f', 4)
                        .arg(normRect.y, 0, 'f', 4)
                        .arg(normRect.width, 0, 'f', 4)
                        .arg(normRect.height, 0, 'f', 4));
                m_view->addEventMessage("info", QString("归一化矩形框已确认并发送！归一化坐标(%.4f,%.4f) 尺寸%.4f×%.4f")
                    .arg(normRect.x, 0, 'f', 4).arg(normRect.y, 0, 'f', 4).arg(normRect.width, 0, 'f', 4).arg(normRect.height, 0, 'f', 4));
            } else {
                QMessageBox::warning(m_view, "区域识别",
                    "归一化矩形框已确认！\n暂无TCP连接，无法发送矩形框信息。");
                m_view->addEventMessage("warning", "归一化矩形框已确认但无TCP连接，未发送");
            }
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
                if (tcpWin && tcpWin->hasConnectedClients()) {
                    int targetCameraId = tcpWin->getCurrentCameraId();
                    tcpWin->Tcp_sent_info(targetCameraId, DEVICE_CAMERA, CAMERA_REGION_ENABLE, 0);
                    tcpWin->Tcp_sent_info(targetCameraId, DEVICE_CAMERA, CAMERA_OBJECT_ENABLE, 0);
                } else {
                    m_view->addEventMessage("warning", "请先连接TCP服务");
                }
                QMessageBox::information(m_view, "区域识别", "AI功能已关闭，区域识别与对象识别功能也已关闭！");
                m_view->addEventMessage("info", "AI功能已关闭，区域识别与对象识别功能也已关闭！");
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
                if (tcpWin && tcpWin->hasConnectedClients()) {
                    int targetCameraId = tcpWin->getCurrentCameraId();
                    tcpWin->Tcp_sent_info(targetCameraId, DEVICE_CAMERA, CAMERA_AI_ENABLE, 1);
                } else {
                    m_view->addEventMessage("warning", "请先连接TCP服务");
                }
                QMessageBox::information(m_view, "AI功能", "区域识别功能需要AI功能支持，已自动开启AI功能！");
                m_view->addEventMessage("info", "区域识别功能需要AI功能支持，已自动开启AI功能！");
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
                if (tcpWin && tcpWin->hasConnectedClients()) {
                    int targetCameraId = tcpWin->getCurrentCameraId();
                    tcpWin->Tcp_sent_info(targetCameraId, DEVICE_CAMERA, CAMERA_AI_ENABLE, 1);
                } else {
                    m_view->addEventMessage("warning", "请先连接TCP服务");
                }
                QMessageBox::information(m_view, "AI功能", "对象识别功能需要AI功能支持，已自动开启AI功能！");
                m_view->addEventMessage("info", "对象识别功能需要AI功能支持，已自动开启AI功能！");
            }
        }
        break;
    }
}

void Controller::onTcpClientConnected(const QString& ip, quint16 port)
{
    QString msg = QString("TCP客户端已连接 IP:%1 端口:%2").arg(ip).arg(port);
    m_view->addEventMessage("info", msg);
    
    // 不再立即提示绑定，等待用户添加摄像头时自动绑定
}

void Controller::onPlanApplied(const PlanData& plan)
{
    qDebug() << "Controller接收到方案应用信号:" << plan.name << "目标摄像头ID:" << plan.cameraId;
    
    // 应用RTSP地址 - 自动启动视频流
    if (!plan.rtspUrl.isEmpty()) {
        m_model->startStream(plan.rtspUrl);
        QString cameraName = (plan.cameraId == 0) ? "主流" : QString("子流%1").arg(plan.cameraId);
        m_view->addEventMessage("info", QString("[%1] 已设置RTSP地址: %2").arg(cameraName).arg(plan.rtspUrl));
    }
    
    // 获取功能按钮列表
    QList<QPushButton*> funButtons = m_view->getFunButtons();
    if (funButtons.size() < 3) {
        m_view->addEventMessage("error", "功能按钮获取失败");
        return;
    }
    
    // 检查TCP连接状态
    if (!(tcpWin && tcpWin->hasConnectedClients())) {
        QMessageBox::warning(m_view, "TCP连接状态", 
            "当前没有TCP连接，部分功能可能无法正常工作。\n请先建立TCP连接。");
        m_view->addEventMessage("warning", "应用方案时检测到没有TCP连接");
        return;
    }
    
    // 使用方案中指定的摄像头ID作为目标
    int targetCameraId = plan.cameraId;
    QString cameraName = (targetCameraId == 0) ? "主流" : QString("子流%1").arg(targetCameraId);
    
    // 应用AI功能设置
    QPushButton* aiBtn = funButtons[0];
    if (aiBtn->isChecked() != plan.aiEnabled) {
        aiBtn->setChecked(plan.aiEnabled);
        if (tcpWin && tcpWin->hasConnectedClients()) {
            tcpWin->Tcp_sent_info(targetCameraId, DEVICE_CAMERA, CAMERA_AI_ENABLE, plan.aiEnabled ? 1 : 0);
        }
        m_view->addEventMessage("info", QString("[%1] AI功能已%2").arg(cameraName).arg(plan.aiEnabled ? "启用" : "禁用"));
    }
    
    // 应用区域识别设置
    QPushButton* regionBtn = funButtons[1];
    if (regionBtn->isChecked() != plan.regionEnabled) {
        regionBtn->setChecked(plan.regionEnabled);
        if (tcpWin && tcpWin->hasConnectedClients()) {
            tcpWin->Tcp_sent_info(targetCameraId, DEVICE_CAMERA, CAMERA_REGION_ENABLE, plan.regionEnabled ? 1 : 0);
        }
        m_view->addEventMessage("info", QString("[%1] 区域识别功能已%2").arg(cameraName).arg(plan.regionEnabled ? "启用" : "禁用"));
    }
    
    // 应用对象识别设置
    QPushButton* objectBtn = funButtons[2];
    if (objectBtn->isChecked() != plan.objectEnabled) {
        objectBtn->setChecked(plan.objectEnabled);
        if (tcpWin && tcpWin->hasConnectedClients()) {
            tcpWin->Tcp_sent_info(targetCameraId, DEVICE_CAMERA, CAMERA_OBJECT_ENABLE, plan.objectEnabled ? 1 : 0);
        }
        m_view->addEventMessage("info", QString("[%1] 对象识别功能已%2").arg(cameraName).arg(plan.objectEnabled ? "启用" : "禁用"));
    }
    
    // 应用对象列表设置
    if (!plan.objectList.isEmpty()) {
        m_selectedObjectIds = plan.objectList;
        
        // 如果对象检测列表窗口已打开，更新其选择状态
        if (m_detectList) {
            m_detectList->setSelectedObjects(m_selectedObjectIds);
        }
        
        // 通过TCP发送对象列表
        if (tcpWin && tcpWin->hasConnectedClients()) {
            tcpWin->Tcp_sent_list(targetCameraId, m_selectedObjectIds);
            
            // 获取对象名称列表用于显示
            QStringList objectNames = DetectList::getObjectNames();
            QStringList selectedNames;
            for (int id : m_selectedObjectIds) {
                if (id >= 0 && id < objectNames.size()) {
                    selectedNames.append(objectNames[id]);
                }
            }
            
            m_view->addEventMessage("info", QString("[%1] 已设置检测对象列表(%2个): %3")
                .arg(cameraName)
                .arg(m_selectedObjectIds.size())
                .arg(selectedNames.join(", ")));
        } else {
            m_view->addEventMessage("warning", QString("[%1] 对象列表已设置，但无TCP连接无法发送").arg(cameraName));
        }
    }
    
    // // 显示应用成功消息
    // QMessageBox::information(m_view, "方案应用成功", 
    //     QString("方案 \"%1\" 已成功应用！\n\n"
    //            "配置详情：\n"
    //            "• RTSP地址: %2\n"
    //            "• AI功能: %3\n"
    //            "• 区域识别: %4\n"
    //            "• 对象识别: %5\n"
    //            "• 检测对象: %6个")
    //     .arg(plan.name)
    //     .arg(plan.rtspUrl)
    //     .arg(plan.aiEnabled ? "启用" : "禁用")
    //     .arg(plan.regionEnabled ? "启用" : "禁用")
    //     .arg(plan.objectEnabled ? "启用" : "禁用")
    //     .arg(plan.objectList.size()));
    
    m_view->addEventMessage("success", QString("方案 \"%1\" 应用成功！").arg(plan.name));
}

void Controller::onDetectionDataReceived(int cameraId, const QString& detectionData)
{
    qDebug() << "Controller接收到检测数据 [摄像头ID:" << cameraId << "]:" << detectionData;
    
    // 记录检测事件到消息系统（包含摄像头信息）
    if (cameraId > 0) {
        m_view->addEventMessage("info", QString("🎯 摄像头%1检测到目标: %2").arg(cameraId).arg(detectionData));
    } else {
        m_view->addEventMessage("info", QString("🎯 检测到目标: %1 (未绑定摄像头)").arg(detectionData));
    }
    
    // 调用报警图像保存函数，传入摄像头ID
    saveAlarmImage(cameraId, detectionData);
}

// ============================================
// 多路视频流管理功能实现
// ============================================

void Controller::addVideoStream(const QString& url, const QString& name, int cameraId)
{
    if (url.isEmpty()) {
        m_view->addEventMessage("warning", "视频流URL为空");
        return;
    }
    
    // 检查摄像头ID是否已被占用
    if (m_view->isCameraIdOccupied(cameraId)) {
        m_view->addEventMessage("warning", QString("摄像头位置 %1 已被占用").arg(cameraId));
        QMessageBox::warning(m_view, "错误", QString("摄像头位置 %1 已被占用！").arg(cameraId));
        return;
    }
    
    // 生成新的流ID
    int streamId = m_nextStreamId++;
    
    // 在View中添加视频流显示（传入摄像头ID）
    m_view->addVideoStream(streamId, name.isEmpty() ? QString("摄像头 %1").arg(cameraId) : name, cameraId);
    
    // 创建新的Model实例
    Model* model = new Model(this);
    
    // 连接帧信号（使用lambda捕获streamId）
    connect(model, &Model::frameReady, this, [this, streamId](const QImage& frame) {
        onModelFrameReady(streamId, frame);
    });
    
    // 连接流断开和重连信号
    connect(model, &Model::streamDisconnected, this, [this, cameraId, name](const QString& url) {
        m_view->addEventMessage("warning", QString("摄像头 %1 (%2) 断开连接").arg(cameraId).arg(name));
    });
    
    connect(model, &Model::streamReconnecting, this, [this, cameraId, name](const QString& url) {
        m_view->addEventMessage("info", QString("摄像头 %1 (%2) 正在尝试重连...").arg(cameraId).arg(name));
    });
    
    // 启动视频流
    model->startStream(url);
    
    // 保存到映射表
    m_streamModels.insert(streamId, model);
    
    // 记录日志
    qDebug() << "添加视频流:" << streamId << "摄像头ID:" << cameraId << "URL:" << url << "Name:" << name;
    m_view->addEventMessage("success", QString("添加摄像头 %1 成功: %2").arg(cameraId).arg(name));
    
    // 自动选中新添加的视频流（这样用户可以立即对其进行操作）
    m_view->selectVideoStream(streamId);
    
    // 不再自动切换布局，保持用户当前选择的布局模式
}

void Controller::removeVideoStream(int streamId)
{
    if (!m_streamModels.contains(streamId)) {
        qDebug() << "警告：尝试删除不存在的视频流ID:" << streamId;
        return;
    }
    
    // 停止并删除Model
    Model* model = m_streamModels.take(streamId);
    model->stopStream();
    model->wait();
    model->deleteLater();
    
    // 从View中移除
    m_view->removeVideoStream(streamId);
    
    // 记录日志
    qDebug() << "删除视频流:" << streamId;
    m_view->addEventMessage("info", QString("视频流 %1 已删除").arg(streamId));
    
    // 不再自动切换布局，保持用户当前选择的布局模式
}

void Controller::clearAllStreams()
{
    // 停止并删除所有Model
    for (auto it = m_streamModels.begin(); it != m_streamModels.end(); ++it) {
        Model* model = it.value();
        model->stopStream();
        model->wait();
        model->deleteLater();
    }
    m_streamModels.clear();
    
    // 清除View中的所有流
    m_view->clearAllStreams();
    
    qDebug() << "已清除所有视频流";
    m_view->addEventMessage("info", "所有视频流已清除");
}

void Controller::onLayoutModeChanged(int mode)
{
    qDebug() << "布局模式切换为:" << mode << "路";
    m_view->switchToLayoutMode(mode);
    m_view->addEventMessage("info", QString("切换到 %1 路显示模式").arg(mode));
}

void Controller::onStreamSelected(int streamId)
{
    if (streamId == -1) {
        qDebug() << "取消选中视频流";
        m_view->addEventMessage("info", "取消选中视频流");
        
        // 取消选中时，清除TCP服务器的当前摄像头ID
        if (tcpWin) {
            tcpWin->setCurrentCameraId(-1);
        }
    } else {
        qDebug() << "选中视频流:" << streamId;
        int cameraId = m_view->getCameraIdForStream(streamId);
        QString streamName = m_view->getStreamName(streamId);
        m_view->addEventMessage("success", QString("已选中：位置%1 - %2").arg(cameraId).arg(streamName));
        
        // 设置TCP服务器的当前摄像头ID（用于定向发送TCP指令）
        if (tcpWin) {
            tcpWin->setCurrentCameraId(cameraId);
            
            // 如果该摄像头已绑定IP，在日志中显示
            QString boundIp = tcpWin->getIpForCamera(cameraId);
            if (!boundIp.isEmpty()) {
                m_view->addEventMessage("info", QString("TCP目标: 摄像头%1 → IP[%2]").arg(cameraId).arg(boundIp));
            }
        }
    }
}

void Controller::onModelFrameReady(int streamId, const QImage& frame)
{
    // 更新指定流的视频帧
    if (!frame.isNull()) {
        m_view->updateVideoFrame(streamId, frame);
    }
}

// 暂停/恢复视频流
void Controller::onStreamPauseRequested(int streamId)
{
    if (!m_streamModels.contains(streamId)) {
        qWarning() << "流" << streamId << "不存在";
        return;
    }
    
    Model* model = m_streamModels.value(streamId);
    
    // 切换暂停/恢复状态
    if (model->isPaused()) {
        model->resumeStream();
        qDebug() << "恢复视频流" << streamId;
        m_view->addEventMessage("success", QString("已恢复视频流 %1").arg(streamId));
    } else {
        model->pauseStream();
        qDebug() << "暂停视频流" << streamId;
        m_view->addEventMessage("info", QString("已暂停视频流 %1").arg(streamId));
    }
}

// 截图视频流
void Controller::onStreamScreenshotRequested(int streamId)
{
    if (!m_streamModels.contains(streamId)) {
        qWarning() << "流" << streamId << "不存在";
        return;
    }
    
    // 获取该流的VideoLabel
    VideoLabel* label = m_view->getVideoLabelForStream(streamId);
    if (!label || label->pixmap() == nullptr) {
        m_view->addEventMessage("warning", "当前流没有可截图的画面");
        QMessageBox::warning(m_view, "提示", "当前流没有可截图的画面！");
        return;
    }
    
    // 获取当前画面
    QPixmap pixmap = *label->pixmap();
    QImage image = pixmap.toImage();
    
    if (image.isNull()) {
        m_view->addEventMessage("warning", "截图失败：图像为空");
        return;
    }
    
    // 获取摄像头ID和名称（通过view层获取）
    int cameraId = m_view->getCameraIdForStream(streamId);
    QString cameraName = m_view->getStreamName(streamId);
    
    if (cameraId == -1) {
        cameraId = streamId; // 如果未找到摄像头ID，使用流ID
    }
    if (cameraName.isEmpty()) {
        cameraName = QString("Camera%1").arg(cameraId);
    }
    
    // 确保picture文件夹存在（使用项目根目录路径，与saveImage保持一致）
    // __FILE__ 在 src/controller/controller.cpp，需要回退到项目根目录
    QString sourcePath = QString(__FILE__).section('/', 0, -4); // 回退到项目根目录
    QDir dir(sourcePath + "/picture/save-picture");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 生成文件名：save_cam{摄像头ID}_{时间精确到秒}_{毫秒}.jpg
    // 格式：save_cam1_20251013_155943_515.jpg
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
    QString filename = QString("save_cam%1_%2.jpg")
                        .arg(cameraId)
                        .arg(timestamp);
    QString filepath = dir.filePath(filename);
    
    // 保存图像
    if (image.save(filepath, "JPEG", 95)) {
        qDebug() << "截图成功:" << filepath;
        m_view->addEventMessage("success", QString("截图成功: %1").arg(filename));
        QMessageBox::information(m_view, "截图成功", "图片已保存到: " + filepath);
    } else {
        qDebug() << "截图失败:" << filepath;
        m_view->addEventMessage("error", "图片保存失败！");
        QMessageBox::critical(m_view, "保存失败", "图片保存失败！");
    }
}

// 从悬停控制条添加指定ID的摄像头
void Controller::onAddCameraWithIdRequested(int cameraId)
{
    // 检查该摄像头ID是否已被占用
    if (m_view->isCameraIdOccupied(cameraId)) {
        QMessageBox::warning(m_view, "错误", QString("摄像头位置 %1 已被占用！").arg(cameraId));
        m_view->addEventMessage("warning", QString("摄像头位置 %1 已被占用").arg(cameraId));
        return;
    }
    
    // 创建固定摄像头ID的对话框（ID不可修改）
    AddCameraDialog dialog(cameraId, m_view);
    
    if (dialog.exec() == QDialog::Accepted) {
        // 获取用户输入的信息
        QString url = dialog.getRtspUrl();
        QString name = dialog.getCameraName();
        
        // 添加视频流
        addVideoStream(url, name, cameraId);
        
        m_view->addEventMessage("success", QString("正在添加摄像头 %1: %2").arg(cameraId).arg(name));
        
        // 自动绑定TCP客户端：检查是否有已连接的TCP客户端
        if (tcpWin && tcpWin->hasConnectedClients()) {
            QStringList connectedIps = tcpWin->getConnectedIps();
            QMap<QString, int> ipCameraMap = tcpWin->getIpCameraMap();
            
            // 查找所有未绑定的IP地址
            QStringList unboundIps;
            for (const QString& ip : connectedIps) {
                if (!ipCameraMap.contains(ip)) {
                    unboundIps.append(ip);
                }
            }
            
            // 如果有未绑定的IP，自动绑定第一个
            if (!unboundIps.isEmpty()) {
                QString selectedIp = unboundIps.first();  // 自动选择第一个未绑定的IP
                
                // 执行绑定
                tcpWin->bindIpToCamera(selectedIp, cameraId);
                m_view->addEventMessage("success", QString("摄像头%1已自动绑定到IP地址%2").arg(cameraId).arg(selectedIp));
                qDebug() << "自动绑定: 摄像头" << cameraId << "→ IP" << selectedIp;
                
                // 自动设置该摄像头为当前操作目标
                tcpWin->setCurrentCameraId(cameraId);
                qDebug() << "已自动设置摄像头" << cameraId << "为当前操作目标";
                
                // 设置VideoLabel显示绑定的IP地址
                m_view->setCameraBoundIp(cameraId, selectedIp);
                qDebug() << "已更新摄像头" << cameraId << "的悬停提示，显示IP:" << selectedIp;
            } else {
                qDebug() << "没有可用的未绑定TCP客户端IP";
                m_view->addEventMessage("info", QString("摄像头%1已添加，但没有可用的TCP客户端进行绑定").arg(cameraId));
            }
        }
    } else {
        // 用户取消了添加操作
        m_view->addEventMessage("info", "取消添加摄像头");
    }
}

