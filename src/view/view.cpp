#include "view.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <QString>
#include <QList>
#include <QGridLayout>
#include <QIcon>
#include <QSize>
#include <QPainter>
#include <QPen>
#include <QRect>
#include <QPoint>
#include <QDebug>
#include <QToolTip>
#include <QTextBrowser>
#include <QDateTime>
#include <QScrollBar>

View::View(QWidget* parent)
    : QWidget(parent)
    , m_hasRectangle(false)
    , m_currentLayoutMode(1)
    , m_fullScreenStreamId(-1)
    , m_selectedStreamId(-1)
    , videoContainer(nullptr)
    , videoGridLayout(nullptr)
    , videoDisplayArea(nullptr)
    , selectedStreamLabel(nullptr)
{  
    // 创建主水平布局
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(3, 3, 3, 3);  // 减少主布局边距适配嵌入式屏幕
    mainLayout->setSpacing(5);  // 减少主布局间距适配嵌入式屏幕

    // 初始化左边布局
    initleft();

    // 初始化中间布局
    initmiddle();

    // 初始化右边布局
    initright();

    // 添加左面板与右面板到主界面水平布局
    mainLayout->addWidget(leftPanel, 1);
    mainLayout->addWidget(middlePanel, 4);
    mainLayout->addWidget(rightPanel, 1);
}

View::~View()
{
    // videoLabel已经作为videoContainer的子控件，会自动释放
}

VideoLabel *View::getVideoLabel() const
{
     return  videoLabel;
}

QList<QPushButton *> View::getTabButtons() const
{
    return  tabButtons;
}

QList<QPushButton*> View::getServoButtons() const
{
    return ServoButtons;
}

QList<QPushButton*> View::getFunButtons() const
{
    return funButtons;
}

// 获取布局切换按钮列表
QList<QPushButton*> View::getLayoutButtons() const
{
    return layoutButtons;
}

// 获取视频流选择下拉框
QComboBox* View::getStreamSelectCombox() const
{
    return streamSelectCombox;
}

void View::initleft()
{
    // 创建左侧面板并应用垂直布局
    leftPanel = new QWidget();
    leftPanel->setMinimumWidth(200); // 减少左侧面板宽度适配嵌入式屏幕
    leftPanel->setMaximumWidth(200); // 减少左侧面板宽度适配嵌入式屏幕
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(3, 10, 3, 3); // 设置边距：左、上、右、下
    
    // 创建事件消息框标签
    eventLabel = new QLabel("事件消息", leftPanel);
    eventLabel->setStyleSheet(
        "QLabel {"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  color: #ffffff;"
        "  background-color: #ff8800;"
        "  border: 1px solid #e67300;"
        "  border-radius: 5px;"
        "  padding: 8px;"
        "  margin: 2px;"
        "}"
    );
    eventLabel->setAlignment(Qt::AlignCenter);
    
    // 创建事件消息文本浏览器
    eventBrowser = new QTextBrowser(leftPanel);
    eventBrowser->setStyleSheet(
        "QTextBrowser {"
        "  font-family: 'Consolas', 'Monaco', monospace;"
        "  font-size: 12px;"
        "  background-color: #ffffff;"
        "  border: 1px solid #cccccc;"
        "  border-radius: 5px;"
        "  padding: 5px;"
        "  selection-background-color: #3399ff;"
        "}"
    );
    eventBrowser->setMinimumHeight(200);
    eventBrowser->setMinimumWidth(140); // 减少文本浏览器宽度适配嵌入式屏幕
    
    
    // 将控件添加到左侧布局
    leftLayout->addWidget(eventLabel);
    leftLayout->addWidget(eventBrowser);
    //leftLayout->addStretch(); // 添加弹性空间
}

void View::initmiddle()
{
    // 创建中间面板并应用垂直布局
    middlePanel = new QWidget();
    QVBoxLayout *middleLayout = new QVBoxLayout(middlePanel);
    middleLayout->setContentsMargins(3, 3, 3, 3); // 减少边距适配嵌入式屏幕
    middleLayout->setSpacing(3); // 减少间距适配嵌入式屏幕

    // 初始化功能按钮面板
    initFunButtons();

    // 初始化多路视频容器（包含videoLabel用于绘框功能）
    initVideoContainer();

    // 创建视频显示区域
    videoDisplayArea = new QWidget();
    QVBoxLayout* displayLayout = new QVBoxLayout(videoDisplayArea);
    displayLayout->setContentsMargins(0, 0, 0, 0);
    displayLayout->setSpacing(0);
    
    // 直接显示多路容器，默认为1路模式
    displayLayout->addWidget(videoContainer);

    // 初始化多路视频流控制面板
    initMultiStreamControl();

    // 将功能按钮面板、视频显示区域和多路控制面板添加到中间布局
    middleLayout->addWidget(funPanel, 1);
    middleLayout->addWidget(videoDisplayArea, 8);
    middleLayout->addWidget(multiStreamPanel, 0);
}

void View::initright()
{
    // 创建右侧面板并应用垂直布局
    rightPanel = new QWidget();
    rightPanel->setMinimumWidth(200); // 减少右侧面板宽度适配嵌入式屏幕
    rightPanel->setMaximumWidth(200); // 减少右侧面板宽度适配嵌入式屏幕
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

    // 初始化右侧按钮面板
    initButtons();
    // 初始化舵机控制面板
    initservo();
    // 初始化步进布局
    initstep();

    // 将按钮面板添加到右侧布局
    rightLayout->addWidget(buttonPanel,6);
    // 将舵机面板添加到右侧布局
    rightLayout->addWidget(servoPanel,2);
    // 将步进面板添加到右侧布局
    rightLayout->addWidget(stepPanel,1);
}

void View::initFunButtons()
{
    // 创建水平布局
    funPanel = new QWidget();
    QHBoxLayout* funLayout = new QHBoxLayout(funPanel);
    funLayout->setContentsMargins(3, 3, 3, 3); // 减少边距适配嵌入式屏幕
    funLayout->setSpacing(3); // 减少按钮间距适配嵌入式屏幕

    // 设置功能面板样式
    funPanel->setStyleSheet(R"(
        QWidget {
            background-color: #f0f0f0;
            border: 1px solid #cccccc;
            border-radius: 5px;
        }
    )");

    // 功能按钮文本 (新增第六个可选按钮: 报警保存)
    QStringList btnNames = {"AI功能", "区域识别", "对象识别", "对象列表", "方案预选", "报警保存"};
    // 图标路径(临时复用 对象列表 图标)
    QStringList tabIconPaths = {
        ":icon/AI.png",
        ":icon/rect.png",
        ":icon/object.png",
        ":icon/list.png",
        ":icon/list.png", // 方案预选 暂用
        ":icon/AI.png"    // 报警保存 暂用 (可以后续替换为专用图标)
    };

    // 功能按钮样式 - 减少padding适配嵌入式屏幕
    QString funBtnStyle = R"(
        QPushButton {
            font-family: "Microsoft YaHei";
            font-size: 14px;
            font-weight: bold;
            background: #4CAF50;
            color: white;
            border: 2px solid #45a049;
            border-radius: 6px;
            padding: 4px 8px;
        }
        QPushButton:hover {
            background: #45a049;
            border-color: #3d8b40;
        }
        QPushButton:pressed {
            background: #3d8b40;
        }
        QPushButton:checked {
            background: #FF9800;
            border-color: #F57C00;
        }
        QPushButton:checked:hover {
            background: #F57C00;
        }
    )";

    funButtons.clear();

    // 遍历按钮名称列表，创建对应的功能按钮
    for (int i = 0; i < btnNames.size(); ++i) {
        QPushButton* btn = new QPushButton(btnNames[i], funPanel); // 创建按钮并设置父级为funPanel
        btn->setStyleSheet(funBtnStyle); // 应用按钮样式
        btn->setProperty("ButtonID", i); // 设置自定义属性用于区分按钮
        btn->setIcon(QIcon(tabIconPaths[i]));                        // 设置按钮图标
        btn->setIconSize(QSize(24, 24));                             // 减少图标大小适配嵌入式屏幕
        btn->setLayoutDirection(Qt::LeftToRight);                    // 图标在左，文字在右
        // 设置按钮的可选属性
        if (i < 3 || i == 5) {
            // 前三个按钮（AI功能、区域识别、对象识别）和第六个按钮（报警保存）为可选按钮
            btn->setCheckable(true);
            btn->setChecked(false);
        } else {
            // 其余按钮为普通按钮
            btn->setCheckable(false);
        }
        // 将按钮添加到布局中
        funLayout->addWidget(btn);
        funButtons << btn; // 将按钮添加到按钮列表
    }

    // 添加弹性空间，让按钮居中
    funLayout->addStretch();

}

// initVideoLabel 已合并到 initVideoContainer 中

void View::initButtons()
{
    buttonPanel = new QWidget();
    QVBoxLayout *buttonLayout = new QVBoxLayout(buttonPanel);

    // 定义按钮的样式表
    QString buttonStyle = R"(
        QPushButton {
            font-family: "Segoe UI";
            font-size: 28px;
            font-weight: bold;
            background: #BBDDFF;
            color: #003366;
            border-radius: 3px;
            qproperty-iconSize: 32px;
            padding-left: 5px;
            padding-right: 5px;
            text-align: center;
        }
        QPushButton:checked {
            background: #99CCFF;
            border-color: #0044AA;
        }
        QPushButton:hover {
            background: #CCEEFF;
        }
    )";

    // 应用样式到按钮面板
    buttonPanel->setStyleSheet(buttonStyle);

    QStringList tabNames = {"添加", "暂停", "截图", "相册", "绘框", "TCP"};
    QStringList tabIconPaths = {
        ":icon/addvideo.png",
        ":icon/closevideo.png",
        ":icon/screenshot.png",
        ":icon/album.png",
        ":icon/draw.png",
        ":icon/tcp.png"
    };

    tabButtons.clear();
    // 循环创建每个功能按钮，并设置其属性
    for (int i = 0; i < tabIconPaths.size(); ++i) {
        QPushButton* btn = new QPushButton(tabNames[i], buttonPanel); // 创建按钮并设置文字
        btn->setIcon(QIcon(tabIconPaths[i]));                        // 设置按钮图标
        btn->setIconSize(QSize(28, 28));                             // 减少图标大小适配嵌入式屏幕
        btn->setMinimumSize(70, 35);                                 // 减少按钮最小尺寸适配嵌入式屏幕
        btn->setProperty("ButtonID", i);                             // 设置自定义属性用于区分按钮
        btn->setLayoutDirection(Qt::LeftToRight);                    // 图标在左，文字在右
        buttonLayout->addWidget(btn);                                // 将按钮添加到布局中
        tabButtons << btn;                                           // 保存按钮指针到列表
    }
}

void View::initservo()
{
    servoPanel = new QWidget();
    servoPanel->setStyleSheet("background:blue;");

    // 云台按钮文本与ID
    QStringList btnNames = {"上", "下", "左", "右", "复位"};
    QStringList iconPaths = {":icon/up.png", ":icon/down.png", ":icon/left.png", ":icon/right.png", ":icon/reset.png"};
    ServoButtons.clear();

    // 云台按钮样式：白底，悬浮变灰色，圆角
    QString servoBtnStyle = R"(
        QPushButton {
            background: white;
            border: 1px solid #CCCCCC;
            border-radius: 8px;
        }
        QPushButton:hover {
            background: #DDDDDD;
        }
    )";

    // 分配动态属性
    for (int i = 0; i < btnNames.size(); ++i) {
        QPushButton* btn = new QPushButton("", servoPanel); // 不设置文字
        btn->setStyleSheet(servoBtnStyle);
        btn->setProperty("ButtonID", i);

        // 设置图标
        QIcon icon(iconPaths[i]);
        btn->setIcon(icon);
        btn->setIconSize(QSize(28, 28)); // 减少图标大小适配嵌入式屏幕
        btn->setMinimumSize(28, 28);     // 减少按钮最小尺寸适配嵌入式屏幕

        ServoButtons << btn;
    }

    // 设置表格布局
    QGridLayout* grid = new QGridLayout(servoPanel);
    grid->addWidget(ServoButtons[0], 0, 1); // 上
    grid->addWidget(ServoButtons[2], 1, 0); // 左
    grid->addWidget(ServoButtons[4], 1, 1); // 复位
    grid->addWidget(ServoButtons[3], 1, 2); // 右
    grid->addWidget(ServoButtons[1], 2, 1); // 下
    grid->setSpacing(5);
    grid->setContentsMargins(0,0,0,0);
}

void View::initstep()
{
    stepPanel = new QWidget();
    QHBoxLayout *stepLayout = new QHBoxLayout(stepPanel);

    stepLabel = new QLabel("步进");
    stepLayout->addWidget(stepLabel);
    
    // 创建水平滑块
    stepSlider = new QSlider(Qt::Horizontal, stepPanel);
    stepSlider->setRange(1, 10);  // 设置范围为1到10
    stepSlider->setValue(5);      // 设置初始值为5
    stepSlider->setMinimumWidth(100);  // 设置最小宽度
    stepLayout->addWidget(stepSlider);
    
    // 创建下拉框
    stepCombox = new QComboBox(stepPanel);
    // 添加1到10的选项
    for (int i = 1; i <= 10; ++i) {
        stepCombox->addItem(QString::number(i));
    }
    stepCombox->setCurrentText("5");  // 设置初始值为5
    stepLayout->addWidget(stepCombox);
    
    // 连接滑块和下拉框的同步信号
    connect(stepSlider, &QSlider::valueChanged, this, [this](int value) {
        // 滑块值改变时，更新下拉框
        stepCombox->setCurrentText(QString::number(value));
    });
    
    connect(stepCombox, static_cast<void(QComboBox::*)(const QString &)>(&QComboBox::currentTextChanged), 
            this, [this](const QString &text) {
        // 下拉框值改变时，更新滑块
        bool ok;
        int value = text.toInt(&ok);
        if (ok && value >= 1 && value <= 10) {
            stepSlider->setValue(value);
        }
    });
}

// 获取步进值，如果stepCombox存在则返回其当前文本转为int，否则返回默认值5
int View::getStepValue() const
{
    if (stepCombox) {
        return stepCombox->currentText().toInt();
    }
    return 5; // 默认值
}

// ================== 绘框相关方法实现 ==================

// 绘制矩形框，保存并设置到videoLabel
void View::drawRectangle(const RectangleBox& rect)
{
    m_rectangle = rect;
    m_hasRectangle = true;
    
    // 如果当前是全屏模式，在全屏的VideoLabel上绘制
    if (m_fullScreenStreamId != -1 && videoLabels.contains(m_fullScreenStreamId)) {
        VideoLabel* fullscreenLabel = videoLabels.value(m_fullScreenStreamId);
        if (fullscreenLabel) {
            fullscreenLabel->setRectangle(rect);
        }
    }
    // 否则在老的videoLabel上绘制（兼容旧代码）
    else if (videoLabel) {
        videoLabel->setRectangle(rect);
    }
}

// 清除矩形框，标记无矩形并清除videoLabel上的框
void View::clearRectangle()
{
    m_hasRectangle = false;
    
    // 如果当前是全屏模式，清除全屏VideoLabel上的框
    if (m_fullScreenStreamId != -1 && videoLabels.contains(m_fullScreenStreamId)) {
        VideoLabel* fullscreenLabel = videoLabels.value(m_fullScreenStreamId);
        if (fullscreenLabel) {
            fullscreenLabel->clearRectangle();
        }
    }
    // 否则清除老的videoLabel上的框（兼容旧代码）
    else if (videoLabel) {
        videoLabel->clearRectangle();
    }
}

// 获取当前videoLabel上的矩形框
RectangleBox View::getCurrentRectangle() const
{
    // 如果当前是全屏模式，从全屏VideoLabel获取矩形框
    if (m_fullScreenStreamId != -1 && videoLabels.contains(m_fullScreenStreamId)) {
        VideoLabel* fullscreenLabel = videoLabels.value(m_fullScreenStreamId);
        if (fullscreenLabel) {
            return fullscreenLabel->getRectangle();
        }
    }
    // 否则从老的videoLabel获取（兼容旧代码）
    else if (videoLabel) {
        return videoLabel->getRectangle();
    }
    
    // 返回空矩形框
    return RectangleBox();
}

// 槽函数：接收绘制完成的矩形框信号，保存并标记有矩形
void View::onRectangleDrawn(const RectangleBox& rect)
{
    m_rectangle = rect;
    m_hasRectangle = true;
    qDebug() << "View接收到矩形框绘制完成信号:" << rect.x << rect.y << rect.width << rect.height;
}

// 辅助函数：计算VideoLabel中实际图像显示区域（去除黑边）
QRect View::getActualImageRect(VideoLabel* label) const
{
    if (!label || !label->pixmap()) {
        return QRect();
    }
    
    // 获取标签尺寸和图像尺寸
    QSize labelSize = label->size();
    QSize pixmapSize = label->pixmap()->size();
    
    if (pixmapSize.isEmpty() || labelSize.isEmpty()) {
        return QRect();
    }
    
    // 计算缩放后的图像尺寸（保持纵横比）
    QSize scaledSize = pixmapSize.scaled(labelSize, Qt::KeepAspectRatio);
    
    // 计算图像在标签中的偏移（居中显示）
    int offsetX = (labelSize.width() - scaledSize.width()) / 2;
    int offsetY = (labelSize.height() - scaledSize.height()) / 2;
    
    // 返回实际图像显示区域
    return QRect(offsetX, offsetY, scaledSize.width(), scaledSize.height());
}

// 槽函数：接收确认的矩形框信号，保存并发出rectangleConfirmed信号通知Controller
void View::onRectangleConfirmed(const RectangleBox& rect)
{
    m_rectangle = rect;
    m_hasRectangle = true;
    qDebug() << "View接收到矩形框确认信号:" << rect.x << rect.y << rect.width << rect.height;
    
    // 获取当前使用的VideoLabel
    VideoLabel* currentLabel = nullptr;
    
    // 如果当前是全屏模式，使用全屏VideoLabel
    if (m_fullScreenStreamId != -1 && videoLabels.contains(m_fullScreenStreamId)) {
        currentLabel = videoLabels.value(m_fullScreenStreamId);
    }
    // 否则使用老的videoLabel（兼容旧代码）
    else if (videoLabel) {
        currentLabel = videoLabel;
    }
    
    // 执行归一化
    NormalizedRectangleBox normRect;
    
    if (currentLabel) {
        // 获取实际图像显示区域（去除黑边）
        QRect imageRect = getActualImageRect(currentLabel);
        
        if (!imageRect.isEmpty()) {
            // 将矩形框坐标转换为相对于实际图像的坐标（去除黑边偏移）
            int relX = rect.x - imageRect.x();
            int relY = rect.y - imageRect.y();
            int relWidth = rect.width;
            int relHeight = rect.height;
            
            // 基于实际图像尺寸进行归一化
            normRect.x = static_cast<float>(relX) / imageRect.width();
            normRect.y = static_cast<float>(relY) / imageRect.height();
            normRect.width = static_cast<float>(relWidth) / imageRect.width();
            normRect.height = static_cast<float>(relHeight) / imageRect.height();
            
            qDebug() << "实际图像区域:" << imageRect;
            qDebug() << "相对图像坐标:" << relX << relY << relWidth << relHeight;
            qDebug() << "归一化坐标:" << normRect.x << normRect.y << normRect.width << normRect.height;
        } else {
            qWarning() << "无法获取实际图像显示区域";
            normRect.x = 0;
            normRect.y = 0;
            normRect.width = 0;
            normRect.height = 0;
        }
    } else {
        qWarning() << "无法获取VideoLabel，归一化失败";
        normRect.x = 0;
        normRect.y = 0;
        normRect.width = 0;
        normRect.height = 0;
    }
    
    // 发射归一化信号
    emit normalizedRectangleConfirmed(normRect, rect);

    // 兼容原有信号
    emit rectangleConfirmed(rect);
}

// 槽函数：接收取消绘制矩形框信号，标记无矩形
void View::onRectangleCancelled()
{
    m_hasRectangle = false;
    qDebug() << "View接收到矩形框取消信号";
}

// 启用或禁用绘制功能
void View::enableDrawing(bool enabled)
{
    // 如果当前是全屏模式，对全屏的VideoLabel启用绘制
    if (m_fullScreenStreamId != -1 && videoLabels.contains(m_fullScreenStreamId)) {
        VideoLabel* fullscreenLabel = videoLabels.value(m_fullScreenStreamId);
        if (fullscreenLabel) {
            fullscreenLabel->setDrawingEnabled(enabled);
            qDebug() << "对全屏视频流" << m_fullScreenStreamId << "启用绘制功能:" << enabled;
        }
    }
    // 否则对老的videoLabel启用（兼容旧代码）
    else if (videoLabel) {
        videoLabel->setDrawingEnabled(enabled);
        qDebug() << "对videoLabel启用绘制功能:" << enabled;
    }
}

// 判断绘制功能是否启用
bool View::isDrawingEnabled() const
{
    // 如果当前是全屏模式，检查全屏的VideoLabel
    if (m_fullScreenStreamId != -1 && videoLabels.contains(m_fullScreenStreamId)) {
        VideoLabel* fullscreenLabel = videoLabels.value(m_fullScreenStreamId);
        if (fullscreenLabel) {
            return fullscreenLabel->isDrawingEnabled();
        }
    }
    // 否则检查老的videoLabel（兼容旧代码）
    else if (videoLabel) {
        return videoLabel->isDrawingEnabled();
    }
    return false;
}

// 添加事件消息到文本浏览器
void View::addEventMessage(const QString& type, const QString& message)
{
    if (!eventBrowser) return;
    
    QString color;
    QString prefix;
    
    // 根据消息类型设置颜色和前缀
    if (type == "error") {
        color = "#cc0000";
        prefix = "[错误]";
    } else if (type == "warning") {
        color = "#ff8800";
        prefix = "[警告]";
    } else if (type == "success") {
        color = "#008000";
        prefix = "[成功]";
    } else if (type == "info") {
        color = "#0066cc";
        prefix = "[信息]";
    } else {
        color = "#333333";
        prefix = "[消息]";
    }
    
    QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString formattedMessage = QString("<span style='color: #666; font-size: 13px;'>[%1]</span> "
                                      "<span style='color: %2; font-weight: bold;'>%3</span> "
                                      "<span style='color: %4;'>%5</span>")
                               .arg(timeStamp)
                               .arg(color)
                               .arg(prefix)
                               .arg(color)
                               .arg(message);
    
    eventBrowser->append(formattedMessage);
    
    // 自动滚动到底部显示最新消息
    eventBrowser->verticalScrollBar()->setValue(eventBrowser->verticalScrollBar()->maximum());
}

// 初始化多路视频流控制面板
void View::initMultiStreamControl()
{
    // 创建多路视频流控制面板
    multiStreamPanel = new QWidget();
    multiStreamPanel->setMaximumHeight(70); // 减少高度适配嵌入式屏幕
    multiStreamPanel->setStyleSheet(R"(
        QWidget {
            background-color: #f5f5f5;
            border: 1px solid #cccccc;
            border-radius: 4px;
        }
    )");
    
    QHBoxLayout* mainLayout = new QHBoxLayout(multiStreamPanel);
    mainLayout->setContentsMargins(2, 2, 2, 2); // 进一步减少边距
    mainLayout->setSpacing(3); // 进一步减少间距
    
    // ========== 左侧：布局切换按钮 ==========
    QLabel* layoutLabel = new QLabel("布局:", multiStreamPanel); // 简化文字
    layoutLabel->setStyleSheet(R"(
        QLabel {
            font-family: "Microsoft YaHei";
            font-size: 12px;
            font-weight: bold;
            color: #333333;
            background: transparent;
            border: none;
        }
    )");
    mainLayout->addWidget(layoutLabel);
    
    // 布局按钮容器
    QWidget* layoutBtnWidget = new QWidget(multiStreamPanel);
    layoutBtnWidget->setStyleSheet("background: transparent; border: none;");
    QHBoxLayout* layoutBtnLayout = new QHBoxLayout(layoutBtnWidget);
    layoutBtnLayout->setContentsMargins(0, 0, 0, 0);
    layoutBtnLayout->setSpacing(2); // 按钮之间用虚线隔离，间距设小一点
    
    // 布局按钮样式（进一步缩小按钮尺寸适配嵌入式屏幕）
    QString layoutBtnStyle = R"(
        QPushButton {
            font-family: "Microsoft YaHei";
            font-size: 11px;
            font-weight: bold;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #ffffff, stop:1 #e8e8e8);
            color: #333333;
            border: 1px solid #999999;
            border-radius: 4px;
            padding: 4px 6px;
            min-width: 35px;
            min-height: 26px;
            max-width: 40px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #e8f4ff, stop:1 #d0e8ff);
            border-color: #4a90e2;
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #c8e0ff, stop:1 #b0d0ff);
        }
        QPushButton:checked {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #4a90e2, stop:1 #357abd);
            color: white;
            border-color: #2e6da4;
        }
        QPushButton:disabled {
            background: #f0f0f0;
            color: #999999;
            border-color: #cccccc;
        }
    )";
    
    // 创建布局切换按钮
    QStringList layoutNames = {"1路", "4路", "9路", "16路"};
    QList<int> layoutValues = {1, 4, 9, 16};
    
    layoutButtons.clear();
    
    for (int i = 0; i < layoutNames.size(); ++i) {
        QPushButton* btn = new QPushButton(layoutNames[i], layoutBtnWidget);
        btn->setStyleSheet(layoutBtnStyle);
        btn->setCheckable(true);
        btn->setProperty("LayoutMode", layoutValues[i]);
        
        // 默认选中1路
        if (i == 0) {
            btn->setChecked(true);
        }
        
        // 连接点击信号
        connect(btn, &QPushButton::clicked, this, [this, btn, layoutValues, i]() {
            // 取消其他按钮的选中状态
            for (QPushButton* otherBtn : layoutButtons) {
                if (otherBtn != btn) {
                    otherBtn->setChecked(false);
                }
            }
            btn->setChecked(true);
            emit layoutModeChanged(layoutValues[i]);
        });
        
        layoutBtnLayout->addWidget(btn);
        layoutButtons << btn;
        
        // 在按钮之间添加虚线分隔符（除了最后一个按钮）
        if (i < layoutNames.size() - 1) {
            QFrame* separator = new QFrame(layoutBtnWidget);
            separator->setFrameShape(QFrame::VLine);
            separator->setFrameShadow(QFrame::Sunken);
            separator->setStyleSheet(R"(
                QFrame {
                    color: #999999;
                    background: transparent;
                    border: none;
                    border-left: 1px dashed #999999;
                    max-width: 1px;
                }
            )");
            layoutBtnLayout->addWidget(separator);
        }
    }
    
    mainLayout->addWidget(layoutBtnWidget);
    
    // 添加间距
    mainLayout->addSpacing(5); // 减少间距
    
    // ========== 添加选中视频流显示标签 ==========
    selectedStreamLabel = new QLabel("未选中", multiStreamPanel);
    selectedStreamLabel->setStyleSheet(R"(
        QLabel {
            font-family: "Microsoft YaHei";
            font-size: 11px;
            font-weight: bold;
            color: #0078d4;
            background: #e7f3ff;
            border: 1px solid #0078d4;
            border-radius: 3px;
            padding: 3px 8px;
            min-width: 60px;
            max-width: 80px;
        }
    )");
    selectedStreamLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(selectedStreamLabel);
    
    // 添加弹性空间
    mainLayout->addStretch();
    
    // ========== 右侧：视频流选择 ==========
    QLabel* streamLabel = new QLabel("放大:", multiStreamPanel); // 简化文字
    streamLabel->setStyleSheet(R"(
        QLabel {
            font-family: "Microsoft YaHei";
            font-size: 12px;
            font-weight: bold;
            color: #333333;
            background: transparent;
            border: none;
        }
    )");
    mainLayout->addWidget(streamLabel);
    
    // 视频流选择下拉框
    streamSelectCombox = new QComboBox(multiStreamPanel);
    streamSelectCombox->setStyleSheet(R"(
        QComboBox {
            font-family: "Microsoft YaHei";
            font-size: 12px;
            background: white;
            border: 1px solid #999999;
            border-radius: 4px;
            padding: 4px 8px;
            min-width: 120px;
            min-height: 26px;
        }
        QComboBox:hover {
            border-color: #4a90e2;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 4px solid #666666;
            margin-right: 4px;
        }
        QComboBox QAbstractItemView {
            background: white;
            border: 1px solid #4a90e2;
            selection-background-color: #4a90e2;
            selection-color: white;
            font-size: 12px;
        }
    )");
    
    // 添加默认选项
    streamSelectCombox->addItem("无 - 多路显示", -1);
    streamSelectCombox->setCurrentIndex(0);
    
    // 连接下拉框选择信号 - 直接切换到1路放大显示
    connect(streamSelectCombox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, [this](int index) {
        if (streamSelectCombox && index >= 0) {
            int streamId = streamSelectCombox->itemData(index).toInt();
            // 直接调用switchToFullScreen进行放大显示
            switchToFullScreen(streamId);
        }
    });
    
    mainLayout->addWidget(streamSelectCombox);
}

// ==================== 多路视频流管理实现 ====================

// 初始化多路视频容器
void View::initVideoContainer()
{
    videoContainer = new QWidget();
    // 使用更深的背景色，增加与边框的对比度
    videoContainer->setStyleSheet("background-color: #0a0a0a;");
    
    videoGridLayout = new QGridLayout(videoContainer);
    videoGridLayout->setSpacing(4); // 减少间距适配嵌入式屏幕
    videoGridLayout->setContentsMargins(4, 4, 4, 4);
    
    // 创建用于绘框功能的videoLabel（只在需要时使用）
    videoLabel = new VideoLabel(videoContainer);
    videoLabel->setMinimumWidth(320); // 减少最小宽度适配嵌入式屏幕
    videoLabel->setAlignment(Qt::AlignCenter);
    videoLabel->setStyleSheet("background-color: #2d2d2d; color: white; font-size: 20px;");
    videoLabel->setText("未添加信号源");
    videoLabel->hide(); // 默认隐藏，只在绘框或单路主显示时使用
    
    // 连接矩形框绘制完成的信号
    connect(videoLabel, &VideoLabel::rectangleDrawn, this, &View::onRectangleDrawn);
    connect(videoLabel, &VideoLabel::rectangleConfirmed, this, &View::onRectangleConfirmed);
    connect(videoLabel, &VideoLabel::rectangleCancelled, this, &View::onRectangleCancelled);
    
    // 初始化为1路显示模式
    updateVideoLayout();
}

// 添加视频流（指定摄像头ID）
void View::addVideoStream(int streamId, const QString& name, int cameraId)
{
    if (videoLabels.contains(streamId)) {
        qDebug() << "视频流" << streamId << "已存在";
        return;
    }
    
    // 检查摄像头ID是否已被占用
    if (cameraIdMap.contains(cameraId)) {
        qWarning() << "摄像头ID" << cameraId << "已被占用";
        return;
    }
    
    // 检查摄像头ID是否在有效范围内 (1-16)
    if (cameraId < 1 || cameraId > 16) {
        qWarning() << "摄像头ID" << cameraId << "超出有效范围 (1-16)";
        return;
    }
    
    // 创建新的VideoLabel
    VideoLabel* label = new VideoLabel(videoContainer);
    label->setMinimumSize(80, 60); // 减少最小尺寸适配嵌入式屏幕，保持4:3比例
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // 自动扩展填充
    label->setAlignment(Qt::AlignCenter);
    label->setScaledContents(false); // 不自动缩放内容，保持比例
    label->setStyleSheet(R"(
        VideoLabel {
            background-color: #000000;
            border: 3px dashed #00d4ff;
            color: #ffffff;
            font-size: 14px;
            border-radius: 6px;
        }
        VideoLabel:hover {
            border: 3px dashed #00ff88;
            background-color: #0a0a0a;
        }
    )");
    label->setText(QString("等待连接...\n%1\n位置: %2").arg(name).arg(cameraId));
    
    // 设置视频流信息并启用悬停控制条
    label->setStreamInfo(cameraId, name, streamId);
    label->setHoverControlEnabled(true);
    
    // 连接悬停控制条的信号（这些信号会被转发到controller处理）
    connect(label, &VideoLabel::addCameraClicked, this, [this, cameraId](int sid) {
        qDebug() << "悬停控制条：请求在位置" << cameraId << "添加摄像头";
        emit addCameraWithIdRequested(cameraId);
    });
    connect(label, &VideoLabel::pauseStreamClicked, this, [this, label](int sid) {
        // 切换暂停状态
        bool currentPaused = label->isPaused();
        label->setPaused(!currentPaused);
        qDebug() << "悬停控制条：切换流（ID:" << sid << "）暂停状态为" << (!currentPaused ? "暂停" : "播放");
        emit streamPauseRequested(sid);
    });
    connect(label, &VideoLabel::screenshotClicked, this, [this](int sid) {
        qDebug() << "悬停控制条：请求截图（流ID:" << sid << "）";
        emit streamScreenshotRequested(sid);
    });
    connect(label, &VideoLabel::closeStreamClicked, this, [this](int sid) {
        qDebug() << "悬停控制条：请求关闭流（ID:" << sid << "）";
        emit streamRemoveRequested(sid); // 发射信号通知Controller处理
    });
    
    // 连接双击选中信号
    connect(label, &VideoLabel::streamDoubleClicked, this, [this](int sid) {
        qDebug() << "双击选中视频流（ID:" << sid << "）";
        selectVideoStream(sid);
    });
    
    // 连接绘框相关信号
    connect(label, &VideoLabel::rectangleDrawn, this, &View::onRectangleDrawn);
    connect(label, &VideoLabel::rectangleConfirmed, this, &View::onRectangleConfirmed);
    connect(label, &VideoLabel::rectangleCancelled, this, &View::onRectangleCancelled);
    qDebug() << "已为视频流" << streamId << "（摄像头" << cameraId << "）连接绘框信号";
    
    // 保存映射关系
    videoLabels.insert(streamId, label);
    streamNames.insert(streamId, name);
    cameraIdMap.insert(cameraId, streamId);       // 摄像头ID -> 流ID
    streamToCameraMap.insert(streamId, cameraId); // 流ID -> 摄像头ID
    
    // 添加到下拉框
    if (streamSelectCombox) {
        streamSelectCombox->addItem(QString("%1 (位置:%2)").arg(name).arg(cameraId), streamId);
    }
    
    // 更新布局
    updateVideoLayout();
    
    qDebug() << "添加视频流:" << streamId << name << "摄像头ID:" << cameraId;
}

// 删除视频流
void View::removeVideoStream(int streamId)
{
    if (!videoLabels.contains(streamId)) {
        return;
    }
    
    // 获取该流对应的摄像头ID并清除映射
    int cameraId = streamToCameraMap.value(streamId, -1);
    if (cameraId != -1) {
        cameraIdMap.remove(cameraId);
        streamToCameraMap.remove(streamId);
        qDebug() << "释放摄像头ID:" << cameraId;
    }
    
    // 删除VideoLabel
    VideoLabel* label = videoLabels.take(streamId);
    videoGridLayout->removeWidget(label);
    label->deleteLater();
    
    // 删除名称映射
    streamNames.remove(streamId);
    
    // 从下拉框移除
    if (streamSelectCombox) {
        int index = streamSelectCombox->findData(streamId);
        if (index != -1) {
            streamSelectCombox->removeItem(index);
        }
    }
    
    // 如果是全屏显示的流，退出全屏
    if (m_fullScreenStreamId == streamId) {
        m_fullScreenStreamId = -1;
        switchToLayoutMode(m_currentLayoutMode);
    }
    
    // 更新布局
    updateVideoLayout();
    
    qDebug() << "删除视频流:" << streamId << "释放摄像头ID:" << cameraId;
}

// 更新视频帧
void View::updateVideoFrame(int streamId, const QImage& frame)
{
    // 统一使用videoLabels中的label显示视频帧
    VideoLabel* label = videoLabels.value(streamId, nullptr);
    if (label && label->isVisible()) {
        QPixmap pixmap = QPixmap::fromImage(frame);
        label->setPixmap(pixmap.scaled(label->size(), 
                                       Qt::KeepAspectRatio, 
                                       Qt::SmoothTransformation));
    }
}

// 获取指定流的VideoLabel
VideoLabel* View::getVideoLabelForStream(int streamId)
{
    // 统一返回videoLabels中的label
    return videoLabels.value(streamId, nullptr);
}

// 切换布局模式
void View::switchToLayoutMode(int mode)
{
    if (mode != 1 && mode != 4 && mode != 9 && mode != 16) {
        qWarning() << "无效的布局模式:" << mode;
        return;
    }
    
    // 如果之前是全屏模式（单路显示），清除所有VideoLabel上的绘制状态和矩形框
    if (m_fullScreenStreamId != -1) {
        qDebug() << "从全屏模式切换到多路显示，清除矩形框";
        
        // 清除所有VideoLabel的绘制功能和矩形框
        for (VideoLabel* label : videoLabels.values()) {
            if (label) {
                label->setDrawingEnabled(false);
                label->clearRectangle();
            }
        }
        
        // 清除老的videoLabel（如果存在）
        if (videoLabel) {
            videoLabel->setDrawingEnabled(false);
            videoLabel->clearRectangle();
        }
        
        // 清除成员变量中的矩形框标记
        m_hasRectangle = false;
        
        addEventMessage("info", "已切换到多路显示，绘框信息已清除");
    }
    
    m_currentLayoutMode = mode;
    m_fullScreenStreamId = -1; // 退出全屏
    
    // 如果退出全屏，重置下拉框为"无-多路显示"
    if (streamSelectCombox) {
        streamSelectCombox->blockSignals(true); // 阻止信号，避免循环触发
        streamSelectCombox->setCurrentIndex(0); // 选择第一项"无-多路显示"
        streamSelectCombox->blockSignals(false);
    }
    
    // 多路容器始终可见
    videoContainer->show();
    
    // 更新布局
    updateVideoLayout();
    
    qDebug() << "切换到布局模式:" << mode << "路";
}

// 切换到单路全屏（通过下拉框选择放大显示某一路）
void View::switchToFullScreen(int streamId)
{
    if (streamId == -1) {
        // -1表示退出全屏，回到之前的多路显示模式
        // 注意：不使用m_currentLayoutMode，因为全屏时它已经是1
        // 需要记录进入全屏前的布局模式
        m_fullScreenStreamId = -1;
        updateVideoLayout();
        qDebug() << "退出全屏显示，恢复多路模式";
        return;
    }
    
    if (!videoLabels.contains(streamId)) {
        qWarning() << "视频流" << streamId << "不存在";
        return;
    }
    
    // 设置全屏流ID
    m_fullScreenStreamId = streamId;
    
    // 切换到1路模式并更新布局（只显示选中的流）
    int oldMode = m_currentLayoutMode;
    m_currentLayoutMode = 1;
    
    // 同步更新布局按钮状态
    if (!layoutButtons.isEmpty()) {
        layoutButtons[0]->setChecked(true); // 1路按钮
        for (int i = 1; i < layoutButtons.size(); ++i) {
            layoutButtons[i]->setChecked(false);
        }
    }
    
    updateVideoLayout();
    
    QString name = streamNames.value(streamId, QString("Stream %1").arg(streamId));
    int cameraId = getCameraIdForStream(streamId);
    qDebug() << "放大显示视频流:" << streamId << "摄像头ID:" << cameraId << "名称:" << name;
    
    // 添加事件消息
    addEventMessage("info", QString("放大显示: 位置%1 - %2").arg(cameraId).arg(name));
}

// 更新视频布局
void View::updateVideoLayout()
{
    if (!videoGridLayout) return;
    
    // 清空当前布局
    while (QLayoutItem* item = videoGridLayout->takeAt(0)) {
        // 不删除widget，只是从布局中移除
        delete item;
    }
    
    // 如果是全屏模式，只显示选中的那一路
    if (m_fullScreenStreamId != -1 && videoLabels.contains(m_fullScreenStreamId)) {
        VideoLabel* fullscreenLabel = videoLabels.value(m_fullScreenStreamId);
        
        // 隐藏所有其他视频流
        for (VideoLabel* label : videoLabels.values()) {
            if (label != fullscreenLabel) {
                label->hide();
            }
        }
        
        // 清除占位符
        for (VideoLabel* placeholder : placeholderLabels) {
            placeholder->deleteLater();
        }
        placeholderLabels.clear();
        
        // 设置1x1布局
        videoGridLayout->setRowStretch(0, 1);
        videoGridLayout->setColumnStretch(0, 1);
        for (int i = 1; i < 4; ++i) {
            videoGridLayout->setRowStretch(i, 0);
            videoGridLayout->setColumnStretch(i, 0);
        }
        
        // 将全屏视频流添加到布局
        videoGridLayout->addWidget(fullscreenLabel, 0, 0);
        fullscreenLabel->show();
        
        qDebug() << "全屏模式：显示视频流" << m_fullScreenStreamId;
        return;
    }
    
    // 计算行列数
    int rows = 1, cols = 1;
    switch (m_currentLayoutMode) {
        case 1:  rows = 1; cols = 1; break;
        case 4:  rows = 2; cols = 2; break;
        case 9:  rows = 3; cols = 3; break;
        case 16: rows = 4; cols = 4; break;
    }
    
    int maxSlots = rows * cols;
    
    // 清理旧的占位符
    for (VideoLabel* placeholder : placeholderLabels) {
        placeholder->deleteLater();
    }
    placeholderLabels.clear();
    
    // 设置行和列的拉伸因子，让网格均匀分布
    // 先清除旧的拉伸因子
    for (int i = 0; i < 4; i++) {
        videoGridLayout->setRowStretch(i, 0);
        videoGridLayout->setColumnStretch(i, 0);
    }
    // 设置当前布局的拉伸因子（每行每列权重相等）
    for (int i = 0; i < rows; ++i) {
        videoGridLayout->setRowStretch(i, 1);
    }
    for (int i = 0; i < cols; ++i) {
        videoGridLayout->setColumnStretch(i, 1);
    }
    
    // 按照摄像头ID (1-16) 显射到网格位置（从上到下、左到右）
    // 位置1 -> (0,0), 位置2 -> (0,1), 位置3 -> (0,2), 位置4 -> (0,3)
    // 位置5 -> (1,0), ... 位置16 -> (3,3)
    
    // 先创建n×n的虚线框架，并根据摄像头ID放置视频流
    for (int cameraId = 1; cameraId <= maxSlots; ++cameraId) {
        // 计算该摄像头ID对应的网格位置（从上到下、左到右）
        int index = cameraId - 1;  // 转换为0-based索引
        int row = index / 4;  // 始终按4列布局计算位置
        int col = index % 4;
        
        // 对于小于4x4的布局，需要调整行列
        if (m_currentLayoutMode < 16) {
            row = index / cols;
            col = index % cols;
        }
        
        // 检查该摄像头ID是否有视频流
        if (cameraIdMap.contains(cameraId)) {
            int streamId = cameraIdMap.value(cameraId);
            VideoLabel* label = videoLabels.value(streamId);
            if (label) {
                videoGridLayout->addWidget(label, row, col);
                label->show();
            }
        } else {
            // 没有视频流，创建占位符（使用VideoLabel以支持悬停控制条）
            VideoLabel* placeholder = new VideoLabel(videoContainer);
            placeholder->setMinimumSize(80, 60); // 减少最小尺寸适配嵌入式屏幕，保持4:3比例
            placeholder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // 自动扩展填充
            placeholder->setAlignment(Qt::AlignCenter);
            placeholder->setStyleSheet(R"(
                VideoLabel {
                    background-color: #0a0a0a;
                    border: 3px dashed #555555;
                    color: #777777;
                    font-size: 13px;
                    border-radius: 6px;
                }
            )");
            placeholder->setText(QString("位置 %1\n等待添加视频流").arg(cameraId));
            
            // 设置空闲位置的流信息（streamId为-1表示无视频流）
            placeholder->setStreamInfo(cameraId, QString("空闲"), -1);
            placeholder->setHoverControlEnabled(true);
            
            // 连接添加摄像头信号
            connect(placeholder, &VideoLabel::addCameraClicked, this, [this, cameraId](int sid) {
                qDebug() << "占位符悬停控制条：请求在位置" << cameraId << "添加摄像头";
                emit addCameraWithIdRequested(cameraId);
            });
            
            videoGridLayout->addWidget(placeholder, row, col);
            placeholderLabels.append(placeholder);
        }
    }
    
    // 隐藏超出当前布局模式范围的视频流
    for (int streamId : videoLabels.keys()) {
        int cameraId = streamToCameraMap.value(streamId, -1);
        if (cameraId > maxSlots) {
            VideoLabel* label = videoLabels.value(streamId);
            if (label) {
                label->hide();
            }
        }
    }
    
    qDebug() << "更新视频布局:" << m_currentLayoutMode << "路, 显示" 
             << videoLabels.size() << "个流，占位符" << placeholderLabels.size() << "个";
}

// 清除所有视频流
void View::clearAllStreams()
{
    // 删除所有VideoLabel
    for (VideoLabel* label : videoLabels.values()) {
        videoGridLayout->removeWidget(label);
        label->deleteLater();
    }
    
    videoLabels.clear();
    streamNames.clear();
    cameraIdMap.clear();      // 清除摄像头ID映射
    streamToCameraMap.clear(); // 清除流ID到摄像头ID的映射
    
    // 清理占位符
    for (VideoLabel* placeholder : placeholderLabels) {
        placeholder->deleteLater();
    }
    placeholderLabels.clear();
    
    // 清空下拉框（保留第一项"无-多路显示"）
    if (streamSelectCombox) {
        while (streamSelectCombox->count() > 1) {
            streamSelectCombox->removeItem(1);
        }
        streamSelectCombox->setCurrentIndex(0);
    }
    
    // 重置状态
    m_fullScreenStreamId = -1;
    m_currentLayoutMode = 1;
    
    // 更新布局，显示默认的1路占位符
    updateVideoLayout();
    
    qDebug() << "清除所有视频流";
}

// 检查摄像头ID是否已被占用
bool View::isCameraIdOccupied(int cameraId) const
{
    return cameraIdMap.contains(cameraId);
}

// 获取可用的摄像头ID列表（1-16）
QList<int> View::getAvailableCameraIds() const
{
    QList<int> availableIds;
    for (int id = 1; id <= 16; ++id) {
        if (!cameraIdMap.contains(id)) {
            availableIds.append(id);
        }
    }
    return availableIds;
}

// 获取视频流对应的摄像头ID
int View::getCameraIdForStream(int streamId) const
{
    return streamToCameraMap.value(streamId, -1);
}

// 获取视频流名称
QString View::getStreamName(int streamId) const
{
    return streamNames.value(streamId, QString());
}

// 获取已使用的摄像头ID列表
QList<int> View::getUsedCameraIds() const
{
    return cameraIdMap.keys();
}

// 根据摄像头ID获取视频流ID
int View::getStreamIdForCamera(int cameraId) const
{
    return cameraIdMap.value(cameraId, -1);
}

// 选中视频流
void View::selectVideoStream(int streamId)
{
    // 如果双击的是已选中的流，则取消选中
    if (m_selectedStreamId == streamId && streamId != -1) {
        // 恢复为默认样式（蓝色虚线框）
        if (videoLabels.contains(streamId)) {
            VideoLabel* label = videoLabels.value(streamId);
            label->setStyleSheet(R"(
                VideoLabel {
                    background-color: #000000;
                    border: 3px dashed #00d4ff;
                    color: #ffffff;
                    font-size: 14px;
                    border-radius: 6px;
                }
                VideoLabel:hover {
                    border: 3px dashed #00ff88;
                    background-color: #0a0a0a;
                }
            )");
        }
        
        // 清除选中状态
        m_selectedStreamId = -1;
        
        // 更新标签显示
        if (selectedStreamLabel) {
            selectedStreamLabel->setText("未选中");
        }
        
        qDebug() << "取消选中视频流:" << streamId;
        
        // 发射取消选中信号
        emit streamSelected(-1);
        return;
    }
    
    // 取消之前选中的视频流的高亮，恢复为默认蓝色虚线框样式
    if (m_selectedStreamId != -1 && videoLabels.contains(m_selectedStreamId)) {
        VideoLabel* oldLabel = videoLabels.value(m_selectedStreamId);
        oldLabel->setStyleSheet(R"(
            VideoLabel {
                background-color: #000000;
                border: 3px dashed #00d4ff;
                color: #ffffff;
                font-size: 14px;
                border-radius: 6px;
            }
            VideoLabel:hover {
                border: 3px dashed #00ff88;
                background-color: #0a0a0a;
            }
        )");
    }
    
    // 更新选中的视频流ID
    m_selectedStreamId = streamId;
    
    // 高亮显示新选中的视频流（绿色实线框）
    if (streamId != -1 && videoLabels.contains(streamId)) {
        VideoLabel* newLabel = videoLabels.value(streamId);
        newLabel->setStyleSheet(R"(
            VideoLabel {
                background-color: black;
                border: 3px solid #00ff88;
            }
            VideoLabel:hover {
                border: 3px solid #00ff88;
                background-color: #0a0a0a;
            }
        )");
        
        // 更新选中视频流标签显示（仅显示ID）
        if (selectedStreamLabel) {
            int cameraId = getCameraIdForStream(streamId);
            selectedStreamLabel->setText(QString("选中: %1").arg(cameraId));
        }
        
        qDebug() << "选中视频流:" << streamId << "摄像头ID:" << getCameraIdForStream(streamId);
        
        // 发射视频流选中信号，通知Controller
        emit streamSelected(streamId);
    } else {
        // 未选中任何流
        if (selectedStreamLabel) {
            selectedStreamLabel->setText("未选中");
        }
        qDebug() << "取消选中视频流";
        
        // 发射取消选中信号
        emit streamSelected(-1);
    }
}

// 设置视频流绑定的IP地址（通过流ID）
void View::setStreamBoundIp(int streamId, const QString& ip)
{
    VideoLabel* label = getVideoLabelForStream(streamId);
    if (label) {
        label->setBoundIp(ip);
        qDebug() << "设置视频流" << streamId << "的绑定IP:" << ip;
    }
}

// 设置摄像头绑定的IP地址（通过摄像头ID）
void View::setCameraBoundIp(int cameraId, const QString& ip)
{
    // 查找该摄像头ID对应的流ID
    int streamId = getStreamIdForCamera(cameraId);
    if (streamId != -1) {
        setStreamBoundIp(streamId, ip);
        qDebug() << "设置摄像头" << cameraId << "的绑定IP:" << ip;
    } else {
        qDebug() << "警告：摄像头" << cameraId << "没有对应的视频流";
    }
}

// 获取指定摄像头的当前帧图像
QImage View::getCurrentFrameForCamera(int cameraId)
{
    // 查找该摄像头ID对应的流ID
    int streamId = getStreamIdForCamera(cameraId);
    if (streamId == -1) {
        qDebug() << "警告：摄像头" << cameraId << "没有对应的视频流";
        return QImage();
    }
    
    // 获取该流的VideoLabel
    VideoLabel* label = getVideoLabelForStream(streamId);
    if (!label || label->pixmap() == nullptr) {
        qDebug() << "警告：摄像头" << cameraId << "的VideoLabel没有图像";
        return QImage();
    }
    
    // 从QPixmap转换为QImage
    QPixmap pixmap = *label->pixmap();
    QImage image = pixmap.toImage();
    
    qDebug() << "获取摄像头" << cameraId << "的当前帧图像，尺寸:" << image.size();
    return image;
}





