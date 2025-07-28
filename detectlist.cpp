#include "detectlist.h"
#include <QApplication>
#include <QStyle>
#include <QDebug>

// COCO数据集80个对象名称
const QStringList DetectList::s_objectNames = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
    "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket", "bottle",
    "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange",
    "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch", "potted plant", "bed",
    "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone", "microwave", "oven",
    "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"
};

DetectList::DetectList(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("对象检测列表 - COCO数据集");
    setMinimumSize(600, 500);
    setMaximumSize(800, 700);
    
    initUI();
    createObjectCheckBoxes();
}

DetectList::~DetectList()
{
}

void DetectList::initUI()
{
    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // 标题
    QLabel* titleLabel = new QLabel("选择要检测的对象：");
    titleLabel->setStyleSheet(R"(
        QLabel {
            font-family: "Microsoft YaHei";
            font-size: 16px;
            font-weight: bold;
            color: #333333;
            padding: 5px;
        }
    )");
    mainLayout->addWidget(titleLabel);

    // 统计标签
    m_countLabel = new QLabel("已选择: 0/80 个对象");
    m_countLabel->setStyleSheet(R"(
        QLabel {
            font-family: "Microsoft YaHei";
            font-size: 12px;
            color: #666666;
            padding: 3px;
        }
    )");
    mainLayout->addWidget(m_countLabel);

    // 滚动区域
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(R"(
        QScrollArea {
            border: 1px solid #cccccc;
            border-radius: 5px;
            background-color: #fafafa;
        }
        QScrollBar:vertical {
            background: #f0f0f0;
            width: 12px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical {
            background: #c0c0c0;
            border-radius: 6px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background: #a0a0a0;
        }
    )");

    // 内容容器
    m_contentWidget = new QWidget();
    m_gridLayout = new QGridLayout(m_contentWidget);
    m_gridLayout->setSpacing(8);
    m_gridLayout->setContentsMargins(15, 15, 15, 15);

    m_scrollArea->setWidget(m_contentWidget);
    mainLayout->addWidget(m_scrollArea);

    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    // 全选按钮
    m_selectAllBtn = new QPushButton("全选");
    m_selectAllBtn->setStyleSheet(R"(
        QPushButton {
            font-family: "Microsoft YaHei";
            font-size: 12px;
            background: #4CAF50;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            min-width: 60px;
        }
        QPushButton:hover {
            background: #45a049;
        }
        QPushButton:pressed {
            background: #3d8b40;
        }
    )");
    connect(m_selectAllBtn, &QPushButton::clicked, this, &DetectList::onSelectAllClicked);

    // 清空按钮
    m_clearAllBtn = new QPushButton("清空");
    m_clearAllBtn->setStyleSheet(R"(
        QPushButton {
            font-family: "Microsoft YaHei";
            font-size: 12px;
            background: #f44336;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            min-width: 60px;
        }
        QPushButton:hover {
            background: #da190b;
        }
        QPushButton:pressed {
            background: #c62828;
        }
    )");
    connect(m_clearAllBtn, &QPushButton::clicked, this, &DetectList::onClearAllClicked);

    // 弹性空间
    buttonLayout->addWidget(m_selectAllBtn);
    buttonLayout->addWidget(m_clearAllBtn);
    buttonLayout->addStretch();

    // 应用按钮
    m_applyBtn = new QPushButton("应用");
    m_applyBtn->setStyleSheet(R"(
        QPushButton {
            font-family: "Microsoft YaHei";
            font-size: 12px;
            background: #2196F3;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 20px;
            min-width: 70px;
        }
        QPushButton:hover {
            background: #1976D2;
        }
        QPushButton:pressed {
            background: #1565C0;
        }
    )");
    connect(m_applyBtn, &QPushButton::clicked, this, &DetectList::onApplyClicked);

    // 取消按钮
    m_cancelBtn = new QPushButton("取消");
    m_cancelBtn->setStyleSheet(R"(
        QPushButton {
            font-family: "Microsoft YaHei";
            font-size: 12px;
            background: #9E9E9E;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 20px;
            min-width: 70px;
        }
        QPushButton:hover {
            background: #757575;
        }
        QPushButton:pressed {
            background: #616161;
        }
    )");
    connect(m_cancelBtn, &QPushButton::clicked, this, &DetectList::onCancelClicked);

    buttonLayout->addWidget(m_applyBtn);
    buttonLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(buttonLayout);
}

void DetectList::createObjectCheckBoxes()
{
    m_checkBoxes.clear();
    
    // 创建80个复选框，每行8个
    int cols = 8;
    for (int i = 0; i < s_objectNames.size(); ++i) {
        QCheckBox* checkBox = new QCheckBox(s_objectNames[i]);
        checkBox->setProperty("objectId", i);
        
        // 设置复选框样式
        checkBox->setStyleSheet(R"(
            QCheckBox {
                font-family: "Microsoft YaHei";
                font-size: 12px;
                color: #333333;
                spacing: 8px;
                padding: 4px;
            }
            QCheckBox::indicator {
                width: 16px;
                height: 16px;
                border: 2px solid #cccccc;
                border-radius: 3px;
                background-color: white;
            }
            QCheckBox::indicator:checked {
                background-color: #4CAF50;
                border-color: #4CAF50;
                image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTIiIGhlaWdodD0iMTIiIHZpZXdCb3g9IjAgMCAxMiAxMiIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KPHBhdGggZD0iTTEwIDNMNC41IDguNUwyIDYiIHN0cm9rZT0id2hpdGUiIHN0cm9rZS13aWR0aD0iMiIgc3Ryb2tlLWxpbmVjYXA9InJvdW5kIiBzdHJva2UtbGluZWpvaW49InJvdW5kIi8+Cjwvc3ZnPgo=);
            }
            QCheckBox::indicator:unchecked:hover {
                border-color: #4CAF50;
            }
        )");
        
        connect(checkBox, &QCheckBox::stateChanged, this, &DetectList::onCheckBoxChanged);
        
        // 添加到网格布局
        int row = i / cols;
        int col = i % cols;
        m_gridLayout->addWidget(checkBox, row, col);
        
        m_checkBoxes.append(checkBox);
    }
}

void DetectList::onSelectAllClicked()
{
    for (QCheckBox* checkBox : m_checkBoxes) {
        checkBox->setChecked(true);
    }
    updateSelectionCount();
}

void DetectList::onClearAllClicked()
{
    for (QCheckBox* checkBox : m_checkBoxes) {
        checkBox->setChecked(false);
    }
    updateSelectionCount();
}

void DetectList::onApplyClicked()
{
    // 收集选中的对象ID
    m_selectedIds.clear();
    for (QCheckBox* checkBox : m_checkBoxes) {
        if (checkBox->isChecked()) {
            int objectId = checkBox->property("objectId").toInt();
            m_selectedIds.insert(objectId);
        }
    }
    
    // 发送选择变化信号
    emit selectionChanged(m_selectedIds);
    
    qDebug() << "应用选择，选中对象数量:" << m_selectedIds.size();
    qDebug() << "选中的对象ID:" << m_selectedIds;
    
    // 关闭窗口
    close();
}

void DetectList::onCancelClicked()
{
    close();
}

void DetectList::onCheckBoxChanged()
{
    updateSelectionCount();
}

void DetectList::updateSelectionCount()
{
    int selectedCount = 0;
    for (QCheckBox* checkBox : m_checkBoxes) {
        if (checkBox->isChecked()) {
            selectedCount++;
        }
    }
    
    m_countLabel->setText(QString("已选择: %1/80 个对象").arg(selectedCount));
}

QSet<int> DetectList::getSelectedObjects() const
{
    return m_selectedIds;
}

void DetectList::setSelectedObjects(const QSet<int>& selectedIds)
{
    m_selectedIds = selectedIds;
    
    // 更新复选框状态
    for (QCheckBox* checkBox : m_checkBoxes) {
        int objectId = checkBox->property("objectId").toInt();
        checkBox->setChecked(selectedIds.contains(objectId));
    }
    
    updateSelectionCount();
}

QStringList DetectList::getObjectNames()
{
    return s_objectNames;
} 