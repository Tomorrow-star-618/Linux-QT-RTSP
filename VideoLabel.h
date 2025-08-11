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

protected:
    // 重写QLabel的绘图事件，用于自定义绘制（如绘制矩形框和按钮）
    void paintEvent(QPaintEvent* event) override;
    // 重写鼠标按下事件，用于开始绘制或处理按钮点击
    void mousePressEvent(QMouseEvent* event) override;
    // 重写鼠标移动事件，用于动态绘制矩形框
    void mouseMoveEvent(QMouseEvent* event) override;
    // 重写鼠标释放事件，用于结束绘制
    void mouseReleaseEvent(QMouseEvent* event) override;

signals:
    // 矩形框绘制完成信号（用于通知外部矩形框已绘制完成）
    void rectangleDrawn(const RectangleBox& rect);
    // 矩形框确认信号（用于通知外部矩形框已确认）
    void rectangleConfirmed(const RectangleBox& rect);
    // 矩形框取消信号（用于通知外部矩形框已取消）
    void rectangleCancelled();

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
    
    // 绘制矩形框
    void drawRectangle(QPainter& painter);
    // 绘制确认和取消按钮
    void drawButtons(QPainter& painter);
    // 更新按钮的位置
    void updateButtonPositions();
    // 判断点是否在按钮区域内
    bool isPointInButton(const QPoint& pos, const QRect& buttonRect) const;
}; 
