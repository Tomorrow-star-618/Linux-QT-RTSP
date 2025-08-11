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
#include "VideoLabel.h"
#include "common.h"

class View : public QWidget {
    Q_OBJECT

public:
    explicit View(QWidget* parent = nullptr);
    ~View();
    QLabel* getVideoLabel() const;               // 获取图像面板
    QList<QPushButton*> getTabButtons() const;   // 获取标签按钮
    QList<QPushButton*> getServoButtons() const; // 获取云台按钮
    QList<QPushButton*> getFunButtons() const;   // 获取功能按钮
    int getStepValue() const;                    // 获取步进值
    
    void drawRectangle(const RectangleBox& rect); // 绘制矩形框
    void clearRectangle();                        // 清除矩形框
    RectangleBox getCurrentRectangle() const;     // 获取当前矩形框
    
    // 启用/禁用绘制功能
    void enableDrawing(bool enabled);
    bool isDrawingEnabled() const;
    
    // 事件消息相关方法
    void addEventMessage(const QString& type, const QString& message);

signals:
    void rectangleConfirmed(const RectangleBox& rect); // 矩形框确认信号
    void normalizedRectangleConfirmed(const NormalizedRectangleBox& rect, const RectangleBox& absRect); // 归一化矩形框信号

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

    QList<QPushButton*> tabButtons;  // 存储所有标签按钮的列表
    QList<QPushButton*> ServoButtons;// 存储舵机所有按钮的列表
    QList<QPushButton*> funButtons;  // 存储摄像头功能按钮的列表

    VideoLabel* videoLabel;    //图像面板（使用自定义VideoLabel）
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

    // 绘框相关成员变量
    RectangleBox m_rectangle;      // 当前绘制的矩形框
    bool m_hasRectangle;           // 是否有矩形框
}; 
