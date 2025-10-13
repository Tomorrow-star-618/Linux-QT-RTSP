#include "detectlist.h"
#include <QApplication>
#include <QStyle>
#include <QDebug>

// COCO数据集80个对象名称
const QStringList DetectList::s_objectNames = {
    // 80类COCO对象名称
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
    "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket", "bottle",
    "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange",
    "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch", "potted plant", "bed",
    "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone", "microwave", "oven",
    "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"
};

// 构造函数，初始化界面
DetectList::DetectList(QWidget* parent)
    : QWidget(parent)
{
    this->resize(800, 820); // 设置窗口大小
    setWindowTitle("对象检测列表 - COCO数据集");
    
    initUI();               // 初始化界面控件
    createObjectCheckBoxes(); // 创建对象复选框
}

DetectList::~DetectList()
{
}

// 初始化界面控件和布局
void DetectList::initUI()
{
    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    // 设置主布局的控件间距为8像素
    mainLayout->setSpacing(8);
    // 设置主布局的边距为12像素，上下左右各12像素
    mainLayout->setContentsMargins(12, 12, 12, 12);

    // 标题标签
    QLabel* titleLabel = new QLabel("选择要检测的对象：");
    titleLabel->setStyleSheet(R"(
        QLabel {
            font-family: "Microsoft YaHei";
            font-size: 16px;
            font-weight: bold;
            color: #333333;
            padding: 3px;
        }
    )");
    mainLayout->addWidget(titleLabel);

    // 搜索输入框
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("搜索对象名称...");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(R"(
        QLineEdit {
            font-family: "Microsoft YaHei";
            font-size: 13px;
            padding: 4px 8px;
            border: 1px solid #cccccc;
            border-radius: 4px;
            background: #f9f9f9;
        }
    )");
    mainLayout->addWidget(m_searchEdit);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &DetectList::onSearchTextChanged);

    // 统计标签，显示已选择数量
    m_countLabel = new QLabel("已选择: 0/80 个对象");
    m_countLabel->setStyleSheet(R"(
        QLabel {
            font-family: "Microsoft YaHei";
            font-size: 12px;
            color: #666666;
            padding: 2px;
        }
    )");
    mainLayout->addWidget(m_countLabel);

    // 滚动区域，包含所有复选框
    m_scrollArea = new QScrollArea();
    // 设置滚动区域内容自适应大小
    m_scrollArea->setWidgetResizable(true);
    // 隐藏水平滚动条
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 仅在需要时显示垂直滚动条
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

    // 内容容器，放置复选框的网格布局
    // 创建内容容器，用于放置复选框的网格布局
    m_contentWidget = new QWidget();
    m_gridLayout = new QGridLayout(m_contentWidget); // 创建网格布局并设置到内容容器
    m_gridLayout->setSpacing(8); // 设置网格间距为8像素
    m_gridLayout->setContentsMargins(15, 15, 15, 15); // 设置网格布局的边距

    m_scrollArea->setWidget(m_contentWidget); // 将内容容器设置为滚动区域的子控件
    mainLayout->addWidget(m_scrollArea); // 将滚动区域添加到主布局

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
    connect(m_selectAllBtn, &QPushButton::clicked, this, &DetectList::onSelectAllClicked); // 绑定全选槽

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
    connect(m_clearAllBtn, &QPushButton::clicked, this, &DetectList::onClearAllClicked); // 绑定清空槽

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
    connect(m_applyBtn, &QPushButton::clicked, this, &DetectList::onApplyClicked); // 绑定应用槽

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
    connect(m_cancelBtn, &QPushButton::clicked, this, &DetectList::onCancelClicked); // 绑定取消槽

    buttonLayout->addWidget(m_applyBtn);
    buttonLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(buttonLayout);
}

// 创建80个对象复选框，并添加到网格布局
void DetectList::createObjectCheckBoxes()
{
    m_checkBoxes.clear();
    
    // 创建80个复选框，每行5个
    int cols = 5;
    for (int i = 0; i < s_objectNames.size(); ++i) {
        QString label = QString("%1. %2").arg(i+1).arg(s_objectNames[i]);
        QCheckBox* checkBox = new QCheckBox(label); // 创建复选框
        checkBox->setProperty("objectId", i);                  // 设置对象ID属性
        
        // 设置复选框样式
        checkBox->setStyleSheet(R"(
            QCheckBox {
                font-family: "Microsoft YaHei";
                font-size: 13px;
                color: #333333;
                spacing: 8px;
                padding: 4px;
                min-width: 110px;
            }
            QCheckBox::indicator {
                width: 18px;
                height: 18px;
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
        
        connect(checkBox, &QCheckBox::stateChanged, this, &DetectList::onCheckBoxChanged); // 绑定状态变化槽
        
        // 添加到网格布局
        int row = i / cols;
        int col = i % cols;
        m_gridLayout->addWidget(checkBox, row, col);
        
        m_checkBoxes.append(checkBox); // 保存指针
    }
}

// 全选按钮槽函数，选中所有复选框
void DetectList::onSelectAllClicked()
{
    for (QCheckBox* checkBox : m_checkBoxes) {
        checkBox->setChecked(true);
    }
    updateSelectionCount(); // 更新统计
}

// 清空按钮槽函数，取消所有复选框
void DetectList::onClearAllClicked()
{
    for (QCheckBox* checkBox : m_checkBoxes) {
        checkBox->setChecked(false);
    }
    updateSelectionCount(); // 更新统计
}

// 应用按钮槽函数，收集选中的对象ID并发送信号
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

// 取消按钮槽函数，关闭窗口
void DetectList::onCancelClicked()
{
    close();
}

// 复选框状态变化槽函数，更新统计标签
void DetectList::onCheckBoxChanged()
{
    updateSelectionCount();
}

// 更新统计标签，显示已选择数量
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

// 获取当前选中的对象ID集合
QSet<int> DetectList::getSelectedObjects() const
{
    return m_selectedIds;
}

// 设置选中的对象ID集合，并同步复选框状态
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

// 获取COCO对象名称列表
QStringList DetectList::getObjectNames()
{
    return s_objectNames;
} 

void DetectList::onSearchTextChanged(const QString& text)
{
    updateSearchHighlight(text);
}

void DetectList::updateSearchHighlight(const QString& text)
{
    QString search = text.trimmed();
    for (int i = 0; i < m_checkBoxes.size(); ++i) {
        QCheckBox* cb = m_checkBoxes[i];
        QString name = cb->text();
        // 提取英文名部分（去掉前面的序号和点）
        int dotIdx = name.indexOf('.');
        QString objName = (dotIdx != -1) ? name.mid(dotIdx + 2) : name;
        if (!search.isEmpty() && objName.startsWith(search, Qt::CaseInsensitive)) {
            cb->setStyleSheet(cb->styleSheet() + "background-color:rgb(81, 211, 81);"); // 浅绿色
        } else {
            // 恢复原样式
            cb->setStyleSheet(R"(
                QCheckBox {
                    font-family: \"Microsoft YaHei\";
                    font-size: 13px;
                    color: #333333;
                    spacing: 8px;
                    padding: 4px;
                    min-width: 110px;
                }
                QCheckBox::indicator {
                    width: 18px;
                    height: 18px;
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
        }
    }
} 
