#pragma once
#include <QLabel>
#include <QPainter>
#include <QMouseEvent>
#include <QRect>
#include "common.h"


class VideoLabel : public QLabel {
    Q_OBJECT

public:
    explicit VideoLabel(QWidget* parent = nullptr);
    
    // 设置绘框的当前状态（是否正在绘制、是否已有矩形框）
    void setDrawingState(bool isDrawing, bool hasRectangle);
    // 设置当前的矩形框（用于外部直接指定一个矩形框显示）
    void setRectangle(const RectangleBox& rect);
    // 清除当前的矩形框（包括绘制状态和显示的矩形）
    void clearRectangle();
    
    // 启用/禁用绘制功能
    void setDrawingEnabled(bool enabled);
    bool isDrawingEnabled() const { return m_drawingEnabled; }
    
    // 获取当前是否正在绘制矩形框
    bool isDrawing() const { return m_isDrawing; }
    // 获取当前是否已有矩形框
    bool hasRectangle() const { return m_hasRectangle; }
    // 获取当前的矩形框数据
    RectangleBox getRectangle() const { return m_rectangle; }
    
    // 设置视频流信息（用于多路显示时的悬停控制条）
    void setStreamInfo(int cameraId, const QString& cameraName, int streamId);
    // 启用/禁用悬停控制条
    void setHoverControlEnabled(bool enabled);
    bool isHoverControlEnabled() const { return m_hoverControlEnabled; }
    
    // 设置/获取暂停状态
    void setPaused(bool paused);
    bool isPaused() const { return m_isPaused; }
    
    // 设置/获取绑定的IP地址
    void setBoundIp(const QString& ip) { m_boundIp = ip; update(); }
    QString getBoundIp() const { return m_boundIp; }

protected:
    // 重写QLabel的绘图事件，用于自定义绘制（如绘制矩形框和按钮）
    void paintEvent(QPaintEvent* event) override;
    // 重写鼠标按下事件，用于开始绘制或处理按钮点击
    void mousePressEvent(QMouseEvent* event) override;
    // 重写鼠标移动事件，用于动态绘制矩形框
    void mouseMoveEvent(QMouseEvent* event) override;
    // 重写鼠标释放事件，用于结束绘制
    void mouseReleaseEvent(QMouseEvent* event) override;
    // 重写鼠标进入事件，用于显示悬停控制条
    void enterEvent(QEvent* event) override;
    // 重写鼠标离开事件，用于隐藏悬停控制条
    void leaveEvent(QEvent* event) override;
    // 重写鼠标双击事件，用于选中视频流
    void mouseDoubleClickEvent(QMouseEvent* event) override;

signals:
    // 矩形框绘制完成信号（用于通知外部矩形框已绘制完成）
    void rectangleDrawn(const RectangleBox& rect);
    // 矩形框确认信号（用于通知外部矩形框已确认）
    void rectangleConfirmed(const RectangleBox& rect);
    // 矩形框取消信号（用于通知外部矩形框已取消）
    void rectangleCancelled();
    
    // 悬停控制条按钮信号
    void addCameraClicked(int streamId);      // 添加摄像头按钮点击
    void pauseStreamClicked(int streamId);    // 暂停按钮点击
    void screenshotClicked(int streamId);     // 截图按钮点击
    void closeStreamClicked(int streamId);    // 关闭按钮点击
    
    // 视频流选中信号
    void streamDoubleClicked(int streamId);   // 双击视频流选中信号

private:
    // 绘框相关成员变量
    RectangleBox m_rectangle;      // 当前绘制的矩形框
    bool m_isDrawing;              // 是否正在绘制
    QPoint m_startPoint;           // 绘制起始点
    QPoint m_endPoint;             // 绘制结束点
    bool m_hasRectangle;           // 是否有矩形框
    bool m_showButtons;            // 是否显示确定取消按钮
    bool m_rectangleConfirmed;     // 矩形框是否已确认
    bool m_drawingEnabled;         // 绘制功能是否启用
    
    // 按钮区域
    QRect m_confirmButtonRect;     // 确定按钮区域
    QRect m_cancelButtonRect;      // 取消按钮区域
    
    // 悬停控制条相关成员变量
    bool m_hoverControlEnabled;    // 是否启用悬停控制条
    bool m_isHovered;              // 鼠标是否悬停在VideoLabel上
    bool m_isPaused;               // 视频流是否暂停
    int m_cameraId;                // 摄像头ID
    QString m_cameraName;          // 摄像头名称
    QString m_boundIp;             // 绑定的IP地址
    int m_streamId;                // 视频流ID
    QRect m_hoverControlRect;      // 悬停控制条区域
    QRect m_addButtonRect;         // 添加按钮区域
    QRect m_pauseButtonRect;       // 暂停按钮区域
    QRect m_screenshotButtonRect;  // 截图按钮区域
    QRect m_closeButtonRect;       // 关闭按钮区域
    
    // 绘制矩形框
    void drawRectangle(QPainter& painter);
    // 绘制确认和取消按钮
    void drawButtons(QPainter& painter);
    // 更新按钮的位置
    void updateButtonPositions();
    // 判断点是否在按钮区域内
    bool isPointInButton(const QPoint& pos, const QRect& buttonRect) const;
    
    // 绘制悬停控制条
    void drawHoverControl(QPainter& painter);
    // 绘制单个悬停控制按钮
    void drawHoverControlButton(QPainter& painter, const QRect& rect, const QString& text, const QColor& color);
    // 更新悬停控制条的位置
    void updateHoverControlPositions();
    // 判断点是否在悬停控制条的某个按钮内
    int getHoverControlButtonAt(const QPoint& pos) const; // 返回：1-添加 2-暂停 3-截图 4-关闭 0-无
};
