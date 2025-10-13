#include "VideoLabel.h"
#include <QToolTip>
#include <QDebug>
#include <QCursor>
#include <QEvent>

// 构造函数，初始化成员变量
VideoLabel::VideoLabel(QWidget* parent)
    : QLabel(parent), m_isDrawing(false), m_hasRectangle(false), 
      m_showButtons(false), m_rectangleConfirmed(false), m_drawingEnabled(false),
      m_hoverControlEnabled(false), m_isHovered(false), m_isPaused(false), 
      m_cameraId(-1), m_boundIp(""), m_streamId(-1)
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

// 设置视频流信息
void VideoLabel::setStreamInfo(int cameraId, const QString& cameraName, int streamId)
{
    m_cameraId = cameraId;
    m_cameraName = cameraName;
    m_streamId = streamId;
}

// 启用/禁用悬停控制条
void VideoLabel::setHoverControlEnabled(bool enabled)
{
    m_hoverControlEnabled = enabled;
    update();
}

// 设置暂停状态
void VideoLabel::setPaused(bool paused)
{
    m_isPaused = paused;
    update(); // 触发重绘，更新按钮图标
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
    
    // 绘制悬停控制条（多路显示时）- 仅在鼠标悬停时显示
    if (m_hoverControlEnabled && m_isHovered) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        drawHoverControl(painter);
    }
}

void VideoLabel::mousePressEvent(QMouseEvent* event)
{
    // 优先处理悬停控制条的按钮点击（只有在悬停时才可点击）
    if (event->button() == Qt::LeftButton && m_hoverControlEnabled && m_isHovered) {
        int buttonIndex = getHoverControlButtonAt(event->pos());
        if (buttonIndex > 0) {
            switch (buttonIndex) {
                case 1: emit addCameraClicked(m_streamId); break;
                case 2: emit pauseStreamClicked(m_streamId); break;
                case 3: emit screenshotClicked(m_streamId); break;
                case 4: emit closeStreamClicked(m_streamId); break;
            }
            return;
        }
    }
    
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
    // 优先检查是否在悬停控制条的按钮上（只有悬停时才检查）
    if (m_hoverControlEnabled && m_isHovered) {
        int buttonIndex = getHoverControlButtonAt(event->pos());
        if (buttonIndex > 0) {
            setCursor(Qt::PointingHandCursor); // 在按钮上显示手型光标
            update(); // 触发重绘以更新按钮的悬停效果
            return;
        }
    }
    
    // 如果正在绘制矩形框
    if (m_isDrawing && m_drawingEnabled) {
        // 将鼠标位置限制在视频区域内
        QRect videoRect = rect();
        QPoint pos = event->pos();
        pos.setX(qBound(0, pos.x(), videoRect.width()));
        pos.setY(qBound(0, pos.y(), videoRect.height()));
        
        m_endPoint = pos;
        update(); // 实时刷新绘制区域
        return;
    }
    
    // 处理绘制模式下的光标
    if (m_drawingEnabled) {
        // 如果鼠标在视频区域内或按钮上
        if (rect().contains(event->pos())) {
            // 如果显示按钮且矩形框未确认
            if (m_showButtons && !m_rectangleConfirmed) {
                // 如果鼠标在"确定"或"取消"按钮上
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
    } else {
        // 非绘制模式，默认箭头光标
        setCursor(Qt::ArrowCursor);
    }
    
    update(); // 更新显示
}

void VideoLabel::mouseReleaseEvent(QMouseEvent* event)
{
    // 如果是左键释放，且当前正在绘制并且绘制功能已启用
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
            // 确认后只显示蓝色边框，不显示填充
            QPen pen(Qt::blue, 2, Qt::SolidLine);
            painter.setPen(pen);
            painter.drawRect(drawRect);
        } else {
            // 未确认时显示半透明填充和实线边框
            QColor fillColor(0, 0, 255, 40); // 蓝色，40%透明度
            painter.fillRect(drawRect, fillColor);
            
            QPen pen(Qt::blue, 2, Qt::SolidLine);
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
    // 绘制“确定”按钮
    painter.setPen(QPen(Qt::white, 2)); // 设置白色边框，线宽为2
    painter.setBrush(QBrush(QColor(0, 150, 0, 200)));  // 设置填充为绿色半透明
    painter.drawRoundedRect(m_confirmButtonRect, 5, 5); // 绘制圆角矩形作为按钮背景
    
    // 绘制“确定”按钮文字
    painter.setPen(Qt::white); // 设置文字颜色为白色
    QFont font = painter.font();
    font.setPointSize(10);     // 设置字体大小
    font.setBold(true);        // 设置字体加粗
    painter.setFont(font);
    
    QRect textRect = painter.fontMetrics().boundingRect("确定"); // 获取“确定”文字的矩形区域
    QPoint textPos = m_confirmButtonRect.center() - QPoint(textRect.width()/2, textRect.height()/2); // 计算文字居中位置
    painter.drawText(textPos, "确定"); // 绘制“确定”文字
    
    // 绘制“取消”按钮
    painter.setPen(QPen(Qt::white, 2)); // 设置白色边框，线宽为2
    painter.setBrush(QBrush(QColor(200, 0, 0, 200)));  // 设置填充为红色半透明
    painter.drawRoundedRect(m_cancelButtonRect, 5, 5); // 绘制圆角矩形作为按钮背景
    
    // 绘制“取消”按钮文字
    painter.setPen(Qt::white); // 设置文字颜色为白色
    textRect = painter.fontMetrics().boundingRect("取消"); // 获取“取消”文字的矩形区域
    textPos = m_cancelButtonRect.center() - QPoint(textRect.width()/2, textRect.height()/2); // 计算文字居中位置
    painter.drawText(textPos, "取消"); // 绘制“取消”文字
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

// 判断给定点是否在指定按钮区域内
bool VideoLabel::isPointInButton(const QPoint& pos, const QRect& buttonRect) const
{
    return buttonRect.contains(pos);
}

// 鼠标进入事件
void VideoLabel::enterEvent(QEvent* event)
{
    QLabel::enterEvent(event);
    if (m_hoverControlEnabled) {
        m_isHovered = true;
        update(); // 触发重绘以显示悬停控制条
    }
}

// 鼠标离开事件
void VideoLabel::leaveEvent(QEvent* event)
{
    QLabel::leaveEvent(event);
    if (m_hoverControlEnabled) {
        m_isHovered = false;
        update(); // 触发重绘以隐藏悬停控制条
    }
}

// 鼠标双击事件
void VideoLabel::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 发射双击信号，通知选中该视频流
        emit streamDoubleClicked(m_streamId);
        qDebug() << "双击VideoLabel，streamId:" << m_streamId << "cameraId:" << m_cameraId;
    }
    QLabel::mouseDoubleClickEvent(event);
}

// 绘制悬停控制条
void VideoLabel::drawHoverControl(QPainter& painter)
{
    updateHoverControlPositions();
    
    // 绘制半透明背景条
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor(0, 0, 0, 180))); // 黑色半透明背景
    painter.drawRect(m_hoverControlRect);
    
    // 根据控制条高度自适应文字大小
    int barHeight = m_hoverControlRect.height();
    int fontSize = qMax(7, qMin(14, static_cast<int>(barHeight / 2.5))); // 字体大小在7-14之间，除以2.5获得更大字体
    
    // 绘制左侧文字信息（摄像头ID和名称）
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPixelSize(fontSize);
    font.setBold(true);
    painter.setFont(font);
    
    // 判断是否有视频流（streamId >= 0 表示有视频流）
    bool hasStream = (m_streamId >= 0);
    
    QString infoText;
    if (hasStream) {
        // 有视频流：显示"位置 X - 摄像头名称 [IP地址]"
        if (!m_boundIp.isEmpty()) {
            infoText = QString("位置 %1 - %2 [%3]").arg(m_cameraId).arg(m_cameraName).arg(m_boundIp);
        } else {
            infoText = QString("位置 %1 - %2").arg(m_cameraId).arg(m_cameraName);
        }
    } else {
        // 无视频流：显示"位置 X - 空闲"
        infoText = QString("位置 %1 - 空闲").arg(m_cameraId);
    }
    
    int leftMargin = qMax(3, barHeight / 8);
    int textX = m_hoverControlRect.x() + leftMargin;
    int textY = m_hoverControlRect.y() + (m_hoverControlRect.height() + painter.fontMetrics().height()) / 2 - 2;
    
    // 计算文字可用宽度（控制条宽度 - 左边距 - 按钮区域宽度 - 右边距）
    int buttonSize = qMax(14, qMin(32, barHeight - 6));
    int buttonSpacing = qMax(2, buttonSize / 6);
    int rightMargin = qMax(3, barHeight / 8);
    
    // 根据是否有视频流计算按钮宽度
    int buttonsWidth;
    if (hasStream) {
        // 有视频流：显示4个按钮（添加、暂停、截图、关闭）
        buttonsWidth = buttonSize * 4 + buttonSpacing * 3 + rightMargin;
    } else {
        // 无视频流：只显示1个按钮（添加）
        buttonsWidth = buttonSize + rightMargin;
    }
    
    int availableWidth = m_hoverControlRect.width() - textX - buttonsWidth - 5;
    
    // 使用省略文字以适应可用宽度
    QString elidedText = painter.fontMetrics().elidedText(infoText, Qt::ElideRight, availableWidth);
    painter.drawText(textX, textY, elidedText);
    
    // 根据是否有视频流绘制不同的按钮
    if (hasStream) {
        // 有视频流：绘制四个按钮
        drawHoverControlButton(painter, m_addButtonRect, "+", QColor(0, 120, 212));      // 蓝色 - 添加
        // 根据暂停状态显示不同图标：暂停时显示播放三角形▶，播放时显示暂停||
        QString pauseIcon = m_isPaused ? "▶" : "||";
        drawHoverControlButton(painter, m_pauseButtonRect, pauseIcon, QColor(255, 140, 0));   // 橙色 - 暂停/播放
        drawHoverControlButton(painter, m_screenshotButtonRect, "□", QColor(34, 139, 34)); // 绿色 - 截图
        drawHoverControlButton(painter, m_closeButtonRect, "×", QColor(220, 53, 69));    // 红色 - 关闭
    } else {
        // 无视频流：只绘制添加按钮（在右侧居中位置）
        drawHoverControlButton(painter, m_addButtonRect, "+", QColor(0, 120, 212));      // 蓝色 - 添加
    }
}

// 辅助函数：绘制单个控制按钮
void VideoLabel::drawHoverControlButton(QPainter& painter, const QRect& rect, const QString& text, const QColor& color)
{
    // 检查鼠标是否悬停在按钮上
    QPoint mousePos = mapFromGlobal(QCursor::pos());
    bool isHovered = rect.contains(mousePos);
    
    // 根据按钮大小自适应圆角半径
    int cornerRadius = qMax(2, qMin(4, rect.width() / 8));
    
    // 绘制按钮背景
    painter.setPen(Qt::NoPen);
    if (isHovered) {
        painter.setBrush(QBrush(color.lighter(120))); // 悬停时更亮
    } else {
        painter.setBrush(QBrush(color));
    }
    painter.drawRoundedRect(rect, cornerRadius, cornerRadius);
    
    // 根据按钮大小自适应字体大小
    // 修改算法：使用更大的字体，确保16路时清晰
    int buttonFontSize = qMax(8, qMin(18, static_cast<int>(rect.height() * 0.65))); // 字体大小为按钮高度的65%
    
    // 绘制按钮文字/图标
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPixelSize(buttonFontSize);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(rect, Qt::AlignCenter, text);
}

// 更新悬停控制条的位置 - 自适应大小
void VideoLabel::updateHoverControlPositions()
{
    // 根据VideoLabel的高度自适应控制条高度
    // 小窗口（如16路）使用较小的高度，大窗口使用较大的高度
    int labelHeight = height();
    // 修改算法：使用更合理的比例，确保16路时也能显示
    int barHeight = qMax(22, qMin(45, labelHeight / 6)); // 高度在22-45之间，除以6而不是8
    
    // 按钮大小也自适应，确保至少14像素
    int buttonSize = qMax(14, qMin(32, barHeight - 6)); // 按钮略小于控制条，最小14
    int buttonSpacing = qMax(2, buttonSize / 6); // 间距为按钮大小的1/6，减少间距
    int rightMargin = qMax(3, barHeight / 8);
    
    // 控制条位于顶部
    m_hoverControlRect = QRect(0, 0, width(), barHeight);
    
    int buttonY = (barHeight - buttonSize) / 2;
    
    // 判断是否有视频流
    bool hasStream = (m_streamId >= 0);
    
    if (hasStream) {
        // 有视频流：四个按钮从右到左排列
        int currentX = width() - rightMargin - buttonSize;
        
        m_closeButtonRect = QRect(currentX, buttonY, buttonSize, buttonSize);
        currentX -= (buttonSize + buttonSpacing);
        
        m_screenshotButtonRect = QRect(currentX, buttonY, buttonSize, buttonSize);
        currentX -= (buttonSize + buttonSpacing);
        
        m_pauseButtonRect = QRect(currentX, buttonY, buttonSize, buttonSize);
        currentX -= (buttonSize + buttonSpacing);
        
        m_addButtonRect = QRect(currentX, buttonY, buttonSize, buttonSize);
    } else {
        // 无视频流：只显示一个添加按钮，位于右侧
        int currentX = width() - rightMargin - buttonSize;
        m_addButtonRect = QRect(currentX, buttonY, buttonSize, buttonSize);
        
        // 其他按钮设置为空矩形（不会被绘制和点击）
        m_pauseButtonRect = QRect();
        m_screenshotButtonRect = QRect();
        m_closeButtonRect = QRect();
    }
}

// 判断点是否在悬停控制条的某个按钮内
int VideoLabel::getHoverControlButtonAt(const QPoint& pos) const
{
    if (m_addButtonRect.contains(pos)) return 1;       // 添加按钮
    if (m_pauseButtonRect.contains(pos)) return 2;     // 暂停按钮
    if (m_screenshotButtonRect.contains(pos)) return 3; // 截图按钮
    if (m_closeButtonRect.contains(pos)) return 4;     // 关闭按钮
    return 0; // 无按钮
}

