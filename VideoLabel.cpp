#include "VideoLabel.h"
#include <QToolTip>
#include <QDebug>

// 构造函数，初始化成员变量
VideoLabel::VideoLabel(QWidget* parent)
    : QLabel(parent), m_isDrawing(false), m_hasRectangle(false), 
      m_showButtons(false), m_rectangleConfirmed(false), m_drawingEnabled(false)
{
    setMouseTracking(true); // 启用鼠标跟踪，便于捕捉鼠标移动事件
}

// 设置绘制状态和是否已有矩形框
void VideoLabel::setDrawingState(bool isDrawing, bool hasRectangle)
{
    m_isDrawing = isDrawing;         // 是否正在绘制
    m_hasRectangle = hasRectangle;   // 是否已有矩形框
    update();                        // 触发重绘
}

// 设置当前矩形框（外部调用，直接指定一个矩形框显示）
void VideoLabel::setRectangle(const RectangleBox& rect)
{
    m_rectangle = rect;      // 保存矩形框数据
    m_hasRectangle = true;   // 标记已有矩形框
    update();                // 触发重绘
}

// 清除当前的矩形框和相关状态
void VideoLabel::clearRectangle()
{
    m_hasRectangle = false;         // 没有矩形框
    m_isDrawing = false;            // 不在绘制状态
    m_showButtons = false;          // 不显示按钮
    m_rectangleConfirmed = false;   // 未确认
    setCursor(Qt::ArrowCursor);     // 恢复鼠标为箭头
    update();                       // 触发重绘
}

void VideoLabel::setDrawingEnabled(bool enabled)
{
    m_drawingEnabled = enabled;
    if (!enabled) {
        // 禁用绘制功能时，清除当前绘制状态
        clearRectangle();
    }
    update();
}

void VideoLabel::paintEvent(QPaintEvent* event)
{
    // 先调用父类的paintEvent绘制视频图像
    QLabel::paintEvent(event);
    
    // 然后在视频上绘制矩形框
    if (m_isDrawing || m_hasRectangle) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        drawRectangle(painter);
        
        // 如果有矩形框且未确认，绘制按钮
        if (m_hasRectangle && !m_rectangleConfirmed && m_showButtons) {
            drawButtons(painter);
        }
    }
}

void VideoLabel::mousePressEvent(QMouseEvent* event)
{
    // 如果按下的是鼠标左键且绘制功能已启用，则进入绘制流程
    if (event->button() == Qt::LeftButton && m_drawingEnabled) {
        // 检查是否点击了按钮
        if (m_showButtons && !m_rectangleConfirmed) {
            if (isPointInButton(event->pos(), m_confirmButtonRect)) {
                // 点击确定按钮
                m_rectangleConfirmed = true;
                m_showButtons = false;
                qDebug() << "矩形框已确认";
                emit rectangleConfirmed(m_rectangle);
                update();
                return;
            } else if (isPointInButton(event->pos(), m_cancelButtonRect)) {
                // 点击取消按钮
                clearRectangle();
                qDebug() << "矩形框已取消";
                emit rectangleCancelled();
                return;
            }
        }
        
        // 如果没有点击按钮，开始绘制
        m_isDrawing = true;
        m_startPoint = event->pos();
        m_endPoint = event->pos();
        
        // 设置鼠标样式为十字光标
        setCursor(Qt::CrossCursor);
        
        update();
    }
}

void VideoLabel::mouseMoveEvent(QMouseEvent* event)
{
    // 如果未启用绘制功能，则将鼠标指针设置为箭头并返回
    if (!m_drawingEnabled) {
        setCursor(Qt::ArrowCursor);
        return;
    }
    
    // 如果正在绘制矩形框
    if (m_isDrawing) {
        // 将鼠标位置限制在视频区域内
        QRect videoRect = rect();
        QPoint pos = event->pos();
        pos.setX(qBound(0, pos.x(), videoRect.width()));
        pos.setY(qBound(0, pos.y(), videoRect.height()));
        
        m_endPoint = pos;
        update(); // 实时刷新绘制区域
    } else {
        // 如果鼠标在视频区域内或按钮上
        if (rect().contains(event->pos())) {
            // 如果显示按钮且矩形框未确认
            if (m_showButtons && !m_rectangleConfirmed) {
                // 如果鼠标在“确定”或“取消”按钮上
                if (isPointInButton(event->pos(), m_confirmButtonRect) || 
                    isPointInButton(event->pos(), m_cancelButtonRect)) {
                    setCursor(Qt::PointingHandCursor);  // 设置为手型光标
                } else {
                    setCursor(Qt::CrossCursor); // 设置为十字光标
                }
            } else {
                setCursor(Qt::CrossCursor); // 设置为十字光标
            }
        } else {
            setCursor(Qt::ArrowCursor); // 设置为箭头光标
        }
    }
}

void VideoLabel::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_isDrawing && m_drawingEnabled) {
        m_isDrawing = false;
        
        // 恢复鼠标样式
        setCursor(Qt::ArrowCursor);
        
        // 限制在视频区域内
        QRect videoRect = rect();
        QPoint pos = event->pos();
        pos.setX(qBound(0, pos.x(), videoRect.width()));
        pos.setY(qBound(0, pos.y(), videoRect.height()));
        
        m_endPoint = pos;
        
        // 计算矩形框
        int x = qMin(m_startPoint.x(), m_endPoint.x());
        int y = qMin(m_startPoint.y(), m_endPoint.y());
        int width = qAbs(m_endPoint.x() - m_startPoint.x());
        int height = qAbs(m_endPoint.y() - m_startPoint.y());
        
        // 只有当矩形足够大时才保存
        if (width > 5 && height > 5) {
            m_rectangle = RectangleBox(x, y, width, height);
            m_hasRectangle = true;
            m_showButtons = true;  // 显示按钮
            m_rectangleConfirmed = false;  // 未确认状态
            qDebug() << "绘制矩形框:" << x << y << width << height;
            
            // 更新按钮位置
            updateButtonPositions();
            
            // 显示绘制完成的消息
            QToolTip::showText(event->globalPos(), 
                QString("矩形框已绘制: %1×%2").arg(width).arg(height), 
                this, QRect(), 2000);
            
            // 发送信号
            emit rectangleDrawn(m_rectangle);
        }
        
        update();
    }
}

void VideoLabel::drawRectangle(QPainter& painter)
{
    if (m_isDrawing) {
        // 绘制正在绘制的矩形 - 使用虚线边框
        int x = qMin(m_startPoint.x(), m_endPoint.x());
        int y = qMin(m_startPoint.y(), m_endPoint.y());
        int width = qAbs(m_endPoint.x() - m_startPoint.x());
        int height = qAbs(m_endPoint.y() - m_startPoint.y());
        
        QRect drawRect(x, y, width, height);
        
        // 设置半透明填充
        QColor fillColor(255, 0, 0, 30); // 红色，30%透明度
        painter.fillRect(drawRect, fillColor);
        
        // 设置虚线边框
        QPen pen(Qt::red, 2, Qt::DashLine);
        pen.setDashPattern({5, 5}); // 5像素实线，5像素空白
        painter.setPen(pen);
        painter.drawRect(drawRect);
        
        // 显示尺寸信息
        if (width > 10 && height > 10) {
            QString sizeText = QString("%1 × %2").arg(width).arg(height);
            QFont font = painter.font();
            font.setPointSize(10);
            painter.setFont(font);
            
            // 计算文本位置（在矩形中心）
            QRect textRect = painter.fontMetrics().boundingRect(sizeText);
            QPoint textPos = drawRect.center() - QPoint(textRect.width()/2, textRect.height()/2);
            
            // 绘制文本背景
            QRect textBgRect = textRect.translated(textPos);
            textBgRect.adjust(-2, -2, 2, 2);
            painter.fillRect(textBgRect, QColor(0, 0, 0, 150));
            
            // 绘制文本
            painter.setPen(Qt::white);
            painter.drawText(textPos, sizeText);
        }
        
    } else if (m_hasRectangle) {
        // 绘制已保存的矩形
        QRect drawRect(m_rectangle.x, m_rectangle.y, m_rectangle.width, m_rectangle.height);
        
        if (m_rectangleConfirmed) {
            // 确认后只显示绿色边框，不显示填充
            QPen pen(Qt::green, 2, Qt::SolidLine);
            painter.setPen(pen);
            painter.drawRect(drawRect);
        } else {
            // 未确认时显示半透明填充和实线边框
            QColor fillColor(0, 255, 0, 40); // 绿色，40%透明度
            painter.fillRect(drawRect, fillColor);
            
            QPen pen(Qt::green, 2, Qt::SolidLine);
            painter.setPen(pen);
            painter.drawRect(drawRect);
            
            // 显示坐标信息
            QString coordText = QString("(%1, %2) %3×%4")
                               .arg(m_rectangle.x)
                               .arg(m_rectangle.y)
                               .arg(m_rectangle.width)
                               .arg(m_rectangle.height);
            
            QFont font = painter.font();
            font.setPointSize(9);
            painter.setFont(font);
            
            // 计算文本位置（在矩形上方）
            QRect textRect = painter.fontMetrics().boundingRect(coordText);
            QPoint textPos = drawRect.topLeft() + QPoint(0, -textRect.height() - 5);
            
            // 绘制文本背景
            QRect textBgRect = textRect.translated(textPos);
            textBgRect.adjust(-3, -2, 3, 2);
            painter.fillRect(textBgRect, QColor(0, 0, 0, 180));
            
            // 绘制文本
            painter.setPen(Qt::white);
            painter.drawText(textPos, coordText);
        }
    }
}

void VideoLabel::drawButtons(QPainter& painter)
{
    // 绘制确定按钮
    painter.setPen(QPen(Qt::white, 2));
    painter.setBrush(QBrush(QColor(0, 150, 0, 200)));  // 绿色半透明
    painter.drawRoundedRect(m_confirmButtonRect, 5, 5);
    
    // 绘制确定按钮文字
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);
    
    QRect textRect = painter.fontMetrics().boundingRect("确定");
    QPoint textPos = m_confirmButtonRect.center() - QPoint(textRect.width()/2, textRect.height()/2);
    painter.drawText(textPos, "确定");
    
    // 绘制取消按钮
    painter.setPen(QPen(Qt::white, 2));
    painter.setBrush(QBrush(QColor(200, 0, 0, 200)));  // 红色半透明
    painter.drawRoundedRect(m_cancelButtonRect, 5, 5);
    
    // 绘制取消按钮文字
    painter.setPen(Qt::white);
    textRect = painter.fontMetrics().boundingRect("取消");
    textPos = m_cancelButtonRect.center() - QPoint(textRect.width()/2, textRect.height()/2);
    painter.drawText(textPos, "取消");
}

void VideoLabel::updateButtonPositions()
{
    if (!m_hasRectangle) return;
    
    // 按钮尺寸
    int buttonWidth = 60;
    int buttonHeight = 30;
    int buttonSpacing = 10;
    
    // 计算按钮位置（在矩形框下方）
    int totalWidth = buttonWidth * 2 + buttonSpacing;
    int startX = m_rectangle.x + (m_rectangle.width - totalWidth) / 2;
    int buttonY = m_rectangle.y + m_rectangle.height + 10;
    
    // 确保按钮在视频区域内
    if (buttonY + buttonHeight > height()) {
        buttonY = m_rectangle.y - buttonHeight - 10;  // 放在矩形框上方
    }
    
    m_confirmButtonRect = QRect(startX, buttonY, buttonWidth, buttonHeight);
    m_cancelButtonRect = QRect(startX + buttonWidth + buttonSpacing, buttonY, buttonWidth, buttonHeight);
}

bool VideoLabel::isPointInButton(const QPoint& pos, const QRect& buttonRect) const
{
    return buttonRect.contains(pos);
} 

