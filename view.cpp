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
    : QWidget(parent), m_hasRectangle(false)
{  
    // 创建主水平布局
    QHBoxLayout* mainLayout = new QHBoxLayout(this);

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
    delete videoLabel;
}

QLabel *View::getVideoLabel() const
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

void View::initleft()
{
    // 创建左侧面板并应用垂直布局
    leftPanel = new QWidget();
    leftPanel->setMinimumWidth(250); // 设置左侧面板最小宽度
    leftPanel->setMaximumWidth(250); // 设置左侧面板最大宽度，避免过宽
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(5, 20, 5, 5); // 设置边距：左、上、右、下
    
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
    eventBrowser->setMinimumWidth(160); // 设置文本浏览器最小宽度
    
    
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

    // 初始化功能按钮面板
    initFunButtons();

    // 初始化视频显示标签
    initVideoLabel();

    // 将功能按钮面板和视频标签添加到中间布局
    middleLayout->addWidget(funPanel, 1);
    middleLayout->addWidget(videoLabel, 8);
}

void View::initright()
{
    // 创建右侧面板并应用垂直布局
    rightPanel = new QWidget();
    rightPanel->setMinimumWidth(250); // 设置右侧面板最小宽度，与左侧保持一致
    rightPanel->setMaximumWidth(250); // 设置右侧面板最大宽度，避免过宽
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

    // 设置功能面板样式
    funPanel->setStyleSheet(R"(
        QWidget {
            background-color: #f0f0f0;
            border: 1px solid #cccccc;
            border-radius: 5px;
        }
    )");

    // 功能按钮文本
    QStringList btnNames = {"AI功能", "区域识别", "对象识别", "对象列表"};
    // 图表路径
    QStringList tabIconPaths = {
        ":icon/AI.png",
        ":icon/rect.png",
        ":icon/object.png",
        ":icon/list.png",
    };

    // 功能按钮样式
    QString funBtnStyle = R"(
        QPushButton {
            font-family: "Microsoft YaHei";
            font-size: 14px;
            font-weight: bold;
            background: #4CAF50;
            color: white;
            border: 2px solid #45a049;
            border-radius: 8px;
            padding: 8px 16px;
            min-width: 80px;
            min-height: 20px;
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
        btn->setIconSize(QSize(32, 32));                             // 设置图标大小
        btn->setLayoutDirection(Qt::LeftToRight);                    // 图标在左，文字在右
        // 设置按钮的可选属性
        if (i < 3) {
            // 前三个按钮（AI功能、区域识别、对象识别）为可选按钮
            btn->setCheckable(true);
            btn->setChecked(false);
        } else {
            // 第四个按钮（对象列表）为普通按钮
            btn->setCheckable(false);
        }
        // 将按钮添加到布局中
        funLayout->addWidget(btn);
        funButtons << btn; // 将按钮添加到按钮列表
    }

    // 添加弹性空间，让按钮居中
    funLayout->addStretch();

}

void View::initVideoLabel()
{
    // 创建左侧视频显示Label（使用自定义VideoLabel）
    videoLabel = new VideoLabel();
    videoLabel->setMinimumWidth(520);
    videoLabel->setAlignment(Qt::AlignCenter);
    videoLabel->setStyleSheet("background-color: #2d2d2d; color: white; font-size: 20px;");
    videoLabel->setText("未添加信号源");
    
    // 连接矩形框绘制完成的信号
    connect(videoLabel, &VideoLabel::rectangleDrawn, this, &View::onRectangleDrawn);
    connect(videoLabel, &VideoLabel::rectangleConfirmed, this, &View::onRectangleConfirmed);
    connect(videoLabel, &VideoLabel::rectangleCancelled, this, &View::onRectangleCancelled);
}

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
        btn->setIconSize(QSize(32, 32));                             // 设置图标大小
        btn->setMinimumSize(80, 40);                                 // 设置按钮最小尺寸
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
        btn->setIconSize(QSize(32, 32)); // 设置图标大小
        btn->setMinimumSize(30, 30);     // 设置按钮最小尺寸

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
    videoLabel->setRectangle(rect);
}

// 清除矩形框，标记无矩形并清除videoLabel上的框
void View::clearRectangle()
{
    m_hasRectangle = false;
    videoLabel->clearRectangle();
}

// 获取当前videoLabel上的矩形框
RectangleBox View::getCurrentRectangle() const
{
    return videoLabel->getRectangle();
}

// 槽函数：接收绘制完成的矩形框信号，保存并标记有矩形
void View::onRectangleDrawn(const RectangleBox& rect)
{
    m_rectangle = rect;
    m_hasRectangle = true;
    qDebug() << "View接收到矩形框绘制完成信号:" << rect.x << rect.y << rect.width << rect.height;
}

// 槽函数：接收确认的矩形框信号，保存并发出rectangleConfirmed信号通知Controller
void View::onRectangleConfirmed(const RectangleBox& rect)
{
    m_rectangle = rect;
    m_hasRectangle = true;
    qDebug() << "View接收到矩形框确认信号:" << rect.x << rect.y << rect.width << rect.height;
    
    // 归一化处理
    int labelW = videoLabel->width();
    int labelH = videoLabel->height();
    NormalizedRectangleBox normRect;
    normRect.x = static_cast<float>(rect.x) / labelW;
    normRect.y = static_cast<float>(rect.y) / labelH;
    normRect.width = static_cast<float>(rect.width) / labelW;
    normRect.height = static_cast<float>(rect.height) / labelH;
    
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
    if (videoLabel) {
        videoLabel->setDrawingEnabled(enabled);
    }
}

// 判断绘制功能是否启用
bool View::isDrawingEnabled() const
{
    if (videoLabel) {
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





