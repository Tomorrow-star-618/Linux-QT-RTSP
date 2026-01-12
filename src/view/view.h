#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QSlider>
#include <QComboBox>
#include <QPainter>
#include <QMouseEvent>
#include <QRect>
#include <QTextBrowser>
#include <QGridLayout>
#include <QMap>
#include "VideoLabel.h"
#include "common.h"

class View : public QWidget {
    Q_OBJECT

public:
    explicit View(QWidget* parent = nullptr);
    ~View();
    VideoLabel* getVideoLabel() const;           // 获取图像面板（用于绘框功能）
    QList<QPushButton*> getTabButtons() const;   // 获取标签按钮
    QList<QPushButton*> getServoButtons() const; // 获取云台按钮
    QList<QPushButton*> getFunButtons() const;   // 获取功能按钮
    QList<QPushButton*> getLayoutButtons() const; // 获取布局切换按钮
    QComboBox* getStreamSelectCombox() const;    // 获取视频流选择下拉框
    int getStepValue() const;                    // 获取步进值
    
    void drawRectangle(const RectangleBox& rect); // 绘制矩形框
    void clearRectangle();                        // 清除矩形框
    RectangleBox getCurrentRectangle() const;     // 获取当前矩形框
    
    // 启用/禁用绘制功能
    void enableDrawing(bool enabled);
    bool isDrawingEnabled() const;
    
    // 事件消息相关方法
    void addEventMessage(const QString& type, const QString& message);
    
    // ========== 多路视频流管理方法 ==========
    void addVideoStream(int streamId, const QString& name, int cameraId);     // 添加视频流（指定摄像头ID）
    void removeVideoStream(int streamId);                       // 删除视频流
    bool isCameraIdOccupied(int cameraId) const;                // 检查摄像头ID是否已被占用
    QList<int> getAvailableCameraIds() const;                   // 获取可用的摄像头ID列表（1-16）
    void updateVideoFrame(int streamId, const QImage& frame);   // 更新视频帧
    VideoLabel* getVideoLabelForStream(int streamId);           // 获取指定流的VideoLabel
    void switchToLayoutMode(int mode);                          // 切换布局模式
    void switchToFullScreen(int streamId);                      // 切换到单路全屏
    void clearAllStreams();                                     // 清除所有视频流
    int getCurrentLayoutMode() const { return m_currentLayoutMode; } // 获取当前布局模式
    int getCameraIdForStream(int streamId) const;               // 获取视频流对应的摄像头ID
    QString getStreamName(int streamId) const;                  // 获取视频流名称
    void selectVideoStream(int streamId);                       // 选中视频流
    int getSelectedStreamId() const { return m_selectedStreamId; } // 获取当前选中的视频流ID
    QList<int> getUsedCameraIds() const;                        // 获取已使用的摄像头ID列表
    int getStreamIdForCamera(int cameraId) const;               // 根据摄像头ID获取视频流ID
    void setStreamBoundIp(int streamId, const QString& ip);     // 设置视频流绑定的IP地址
    void setCameraBoundIp(int cameraId, const QString& ip);     // 设置摄像头绑定的IP地址（通过摄像头ID）
    QImage getCurrentFrameForCamera(int cameraId);              // 获取指定摄像头的当前帧图像

signals:
    void rectangleConfirmed(const RectangleBox& rect); // 矩形框确认信号
    void normalizedRectangleConfirmed(const NormalizedRectangleBox& rect, const RectangleBox& absRect); // 归一化矩形框信号
    void layoutModeChanged(int mode); // 布局模式切换信号 (1,4,9,16)
    void streamSelected(int streamId); // 视频流选中信号
    void streamPauseRequested(int streamId); // 请求暂停流
    void streamScreenshotRequested(int streamId); // 请求截图流
    void streamRemoveRequested(int streamId); // 请求删除流
    void addCameraWithIdRequested(int cameraId); // 请求添加指定ID的摄像头

private slots:
    void onRectangleDrawn(const RectangleBox& rect); // 处理矩形框绘制完成
    void onRectangleConfirmed(const RectangleBox& rect); // 处理矩形框确认
    void onRectangleCancelled(); // 处理矩形框取消

private:
    void initleft();       // 初始化左边面板
    void initmiddle();     // 初始化中间面板
    void initright();      // 初始化右边面板
    void initButtons();    // 初始化按键列表
    void initservo();      // 初始化云台按键列表 
    void initstep();       // 初始化步进布局
    void initFunButtons(); // 初始化功能按钮
    void initVideoLabel(); // 初始化视频标签
    void initMultiStreamControl(); // 初始化多路视频流控制面板
    void initVideoContainer();     // 初始化多路视频容器
    void updateVideoLayout();      // 更新视频布局
    QRect getActualImageRect(VideoLabel* label) const; // 计算VideoLabel中实际图像显示区域（去除黑边）

    QList<QPushButton*> tabButtons;  // 存储所有标签按钮的列表
    QList<QPushButton*> ServoButtons;// 存储舵机所有按钮的列表
    QList<QPushButton*> funButtons;  // 存储摄像头功能按钮的列表
    QList<QPushButton*> layoutButtons; // 存储布局切换按钮的列表

    VideoLabel* videoLabel;    //图像面板（使用自定义VideoLabel）- 用于单路显示
    QLabel* stepLabel;         //步进标签
    QSlider* stepSlider;       //步进滑块
    QComboBox* stepCombox;     //步进下拉框
    QLabel* eventLabel;        //事件消息框标签
    QTextBrowser* eventBrowser; //事件消息显示框

    QWidget* leftPanel;    //左边整体面板
    QWidget* funPanel;     //中上方功能面板
    QWidget* middlePanel;  //中间整体面板
    QWidget* rightPanel;   //右边整体面板
    QWidget* buttonPanel;  //按键整体面板
    QWidget* servoPanel;   //云台整体面板
    QWidget* stepPanel;    //步进整体面板
    QWidget* multiStreamPanel; // 多路视频流控制面板
    QComboBox* streamSelectCombox; // 视频流选择下拉框
    QLabel* selectedStreamLabel;   // 显示当前选中的视频流标签
    int m_selectedStreamId;        // 当前选中的视频流ID (-1表示未选中)

    // ========== 多路视频流相关成员 ==========
    QWidget* videoContainer;           // 视频显示容器
    QGridLayout* videoGridLayout;      // 网格布局
    QMap<int, VideoLabel*> videoLabels;// 视频流ID -> VideoLabel映射
    QMap<int, QString> streamNames;    // 视频流ID -> 名称映射
    QMap<int, int> cameraIdMap;        // 摄像头ID (1-16) -> 视频流ID的映射
    QMap<int, int> streamToCameraMap;  // 视频流ID -> 摄像头ID (1-16) 的映射
    QList<VideoLabel*> placeholderLabels;  // 占位符标签列表（显示虚线框架，使用VideoLabel支持悬停控制条）
    int m_currentLayoutMode;           // 当前布局模式 (1,4,9,16)
    int m_fullScreenStreamId;          // 全屏显示的流ID (-1表示无)
    QWidget* videoDisplayArea;         // 视频显示区域（放置videoLabel或videoContainer）

    // 绘框相关成员变量
    RectangleBox m_rectangle;      // 当前绘制的矩形框
    bool m_hasRectangle;           // 是否有矩形框
};
