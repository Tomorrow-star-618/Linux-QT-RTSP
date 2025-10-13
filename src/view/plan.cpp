#include "plan.h"
#include "detectlist.h"
#include <QApplication>
#include <QHeaderView>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

Plan::Plan(QWidget *parent)
    : QDialog(parent)
    , m_currentPlanIndex(-1)
    , m_formModified(false)
{
    setWindowTitle("方案预选");
    setWindowIcon(QIcon(":icon/list.png"));
    resize(800, 600);
    
    // 初始化界面
    initUI();
    
    // 初始化数据库
    initDatabase();
    
    // 加载数据
    loadPlansFromDatabase();
    
    // 如果没有方案，创建默认方案
    if (m_plans.isEmpty()) {
        createDefaultPlans();
    }
    
    // 更新界面
    updatePlanList();
}

Plan::~Plan()
{
    // 关闭数据库连接
    if (m_database.isOpen()) {
        m_database.close();
    }
}

void Plan::initUI()
{
    // 创建主布局
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    
    // 创建分割器
    m_splitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(m_splitter);
    
    // === 左侧方案列表区域 ===
    QWidget* leftWidget = new QWidget();
    leftWidget->setMinimumWidth(250);
    leftWidget->setMaximumWidth(300);
    
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    
    // 方案列表标题
    QLabel* listLabel = new QLabel("方案列表");
    listLabel->setStyleSheet(
        "QLabel {"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "  color: #333333;"
        "  padding: 5px;"
        "}"
    );
    leftLayout->addWidget(listLabel);
    
    // 方案列表
    m_planListWidget = new QListWidget();
    m_planListWidget->setStyleSheet(
        "QListWidget {"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 14px;"
        "  border: 1px solid #cccccc;"
        "  border-radius: 5px;"
        "  background-color: white;"
        "  selection-background-color: #4CAF50;"
        "  selection-color: white;"
        "}"
        "QListWidget::item {"
        "  padding: 8px;"
        "  border-bottom: 1px solid #eeeeee;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #f0f8ff;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "}"
    );
    leftLayout->addWidget(m_planListWidget);
    
    // 操作按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_newButton = new QPushButton("新建");
    m_newButton->setStyleSheet(
        "QPushButton {"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 12px;"
        "  background: #4CAF50;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 6px 12px;"
        "  min-width: 60px;"
        "}"
        "QPushButton:hover { background: #45a049; }"
        "QPushButton:pressed { background: #3d8b40; }"
    );
    
    m_deleteButton = new QPushButton("删除");
    m_deleteButton->setStyleSheet(
        "QPushButton {"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 12px;"
        "  background: #f44336;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 6px 12px;"
        "  min-width: 60px;"
        "}"
        "QPushButton:hover { background: #da190b; }"
        "QPushButton:pressed { background: #c0110f; }"
    );
    m_deleteButton->setEnabled(false);
    
    buttonLayout->addWidget(m_newButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addStretch();
    
    leftLayout->addLayout(buttonLayout);
    
    // === 右侧配置区域 ===
    QWidget* rightWidget = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    
    // 配置表单
    QGroupBox* configGroup = new QGroupBox("方案配置");
    configGroup->setStyleSheet(
        "QGroupBox {"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  color: #333333;"
        "  border: 2px solid #cccccc;"
        "  border-radius: 5px;"
        "  margin-top: 10px;"
        "  padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 10px;"
        "  padding: 0 5px 0 5px;"
        "}"
    );
    
    QGridLayout* configLayout = new QGridLayout(configGroup);
    
    // 方案名称
    configLayout->addWidget(new QLabel("方案名称:"), 0, 0);
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("请输入方案名称");
    configLayout->addWidget(m_nameEdit, 0, 1);
    
    // RTSP地址
    configLayout->addWidget(new QLabel("RTSP地址:"), 1, 0);
    m_rtspEdit = new QLineEdit();
    m_rtspEdit->setPlaceholderText("rtsp://192.168.1.130/live/0");
    configLayout->addWidget(m_rtspEdit, 1, 1);
    
    // 功能使能
    configLayout->addWidget(new QLabel("功能配置:"), 2, 0);
    QVBoxLayout* checkBoxLayout = new QVBoxLayout();
    
    m_aiCheckBox = new QCheckBox("AI识别功能");
    m_regionCheckBox = new QCheckBox("区域识别功能");
    m_objectCheckBox = new QCheckBox("对象识别功能");
    
    checkBoxLayout->addWidget(m_aiCheckBox);
    checkBoxLayout->addWidget(m_regionCheckBox);
    checkBoxLayout->addWidget(m_objectCheckBox);
    
    QWidget* checkBoxWidget = new QWidget();
    checkBoxWidget->setLayout(checkBoxLayout);
    configLayout->addWidget(checkBoxWidget, 2, 1);
    
    // 对象列表
    configLayout->addWidget(new QLabel("对象列表:"), 3, 0, Qt::AlignTop);
    QVBoxLayout* objectLayout = new QVBoxLayout();
    
    m_objectListEdit = new QTextEdit();
    m_objectListEdit->setMaximumHeight(100);
    m_objectListEdit->setReadOnly(true);
    m_objectListEdit->setPlaceholderText("点击下方按钮选择检测对象");
    
    m_selectObjectButton = new QPushButton("选择检测对象");
    m_selectObjectButton->setStyleSheet(
        "QPushButton {"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 12px;"
        "  background: #2196F3;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 6px 12px;"
        "}"
        "QPushButton:hover { background: #1976D2; }"
        "QPushButton:pressed { background: #1565C0; }"
    );
    
    objectLayout->addWidget(m_objectListEdit);
    objectLayout->addWidget(m_selectObjectButton);
    
    QWidget* objectWidget = new QWidget();
    objectWidget->setLayout(objectLayout);
    configLayout->addWidget(objectWidget, 3, 1);
    
    rightLayout->addWidget(configGroup);
    
    // 底部操作按钮
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    
    m_saveButton = new QPushButton("保存方案");
    m_saveButton->setStyleSheet(
        "QPushButton {"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 14px;"
        "  background: #FF9800;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 8px 16px;"
        "  min-width: 80px;"
        "}"
        "QPushButton:hover { background: #F57C00; }"
        "QPushButton:pressed { background: #E65100; }"
    );
    m_saveButton->setEnabled(false);
    
    m_applyButton = new QPushButton("应用方案");
    m_applyButton->setStyleSheet(
        "QPushButton {"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 14px;"
        "  background: #4CAF50;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 8px 16px;"
        "  min-width: 80px;"
        "}"
        "QPushButton:hover { background: #45a049; }"
        "QPushButton:pressed { background: #3d8b40; }"
    );
    m_applyButton->setEnabled(false);
    
    bottomLayout->addWidget(m_saveButton);
    bottomLayout->addWidget(m_applyButton);
    
    rightLayout->addLayout(bottomLayout);
    
    // 添加到分割器
    m_splitter->addWidget(leftWidget);
    m_splitter->addWidget(rightWidget);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    
    // 连接信号槽
    connect(m_planListWidget, &QListWidget::currentRowChanged, this, &Plan::onPlanListSelectionChanged);
    connect(m_newButton, &QPushButton::clicked, this, &Plan::onNewPlan);
    connect(m_deleteButton, &QPushButton::clicked, this, &Plan::onDeletePlan);
    connect(m_saveButton, &QPushButton::clicked, this, &Plan::onSavePlan);
    connect(m_applyButton, &QPushButton::clicked, this, &Plan::onApplyPlan);
    connect(m_selectObjectButton, &QPushButton::clicked, this, [this]() {
        // 打开对象选择对话框
        DetectList* detectList = new DetectList();
        detectList->setAttribute(Qt::WA_DeleteOnClose);
        
        // 获取当前选中的对象列表
        PlanData currentPlan;
        updatePlanFromForm(currentPlan);
        detectList->setSelectedObjects(currentPlan.objectList);
        
        // 连接选择变化信号
        connect(detectList, &DetectList::selectionChanged, this, [this](const QSet<int>& selectedIds) {
            // 更新当前表单的对象列表
            if (m_currentPlanIndex >= 0 && m_currentPlanIndex < m_plans.size()) {
                m_plans[m_currentPlanIndex].objectList = selectedIds;
                
                // 更新显示
                QStringList objectNames = DetectList::getObjectNames();
                QStringList selectedNames;
                for (int id : selectedIds) {
                    if (id >= 0 && id < objectNames.size()) {
                        selectedNames.append(objectNames[id]);
                    }
                }
                m_objectListEdit->setText(selectedNames.join(", "));
                
                // 标记为已修改
                m_formModified = true;
                m_saveButton->setEnabled(true);
            }
        });
        
        detectList->show();
    });
    
    // 表单数据变化监听
    connect(m_nameEdit, &QLineEdit::textChanged, this, &Plan::onFormDataChanged);
    connect(m_rtspEdit, &QLineEdit::textChanged, this, &Plan::onFormDataChanged);
    connect(m_aiCheckBox, &QCheckBox::toggled, this, &Plan::onFormDataChanged);
    connect(m_regionCheckBox, &QCheckBox::toggled, this, &Plan::onFormDataChanged);
    connect(m_objectCheckBox, &QCheckBox::toggled, this, &Plan::onFormDataChanged);
    
    // 设置统一的输入框样式
    QString lineEditStyle = 
        "QLineEdit {"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 12px;"
        "  border: 1px solid #cccccc;"
        "  border-radius: 4px;"
        "  padding: 6px;"
        "  background-color: white;"
        "}"
        "QLineEdit:focus {"
        "  border-color: #3399ff;"
        "}";
    
    m_nameEdit->setStyleSheet(lineEditStyle);
    m_rtspEdit->setStyleSheet(lineEditStyle);
    
    QString checkBoxStyle =
        "QCheckBox {"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 12px;"
        "  color: #333333;"
        "  spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "  width: 16px;"
        "  height: 16px;"
        "}"
        "QCheckBox::indicator:unchecked {"
        "  border: 2px solid #cccccc;"
        "  border-radius: 3px;"
        "  background-color: white;"
        "}"
        "QCheckBox::indicator:checked {"
        "  border: 2px solid #4CAF50;"
        "  border-radius: 3px;"
        "  background-color: #4CAF50;"
        "  image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTIiIGhlaWdodD0iMTIiIHZpZXdCb3g9IjAgMCAxMiAxMiIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KPHBhdGggZD0iTTEwIDNMNC41IDguNUwyIDYiIHN0cm9rZT0id2hpdGUiIHN0cm9rZS13aWR0aD0iMiIgc3Ryb2tlLWxpbmVjYXA9InJvdW5kIiBzdHJva2UtbGluZWpvaW49InJvdW5kIi8+Cjwvc3ZnPgo=);"
        "}";
    
    m_aiCheckBox->setStyleSheet(checkBoxStyle);
    m_regionCheckBox->setStyleSheet(checkBoxStyle);
    m_objectCheckBox->setStyleSheet(checkBoxStyle);
    
    m_objectListEdit->setStyleSheet(
        "QTextEdit {"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 12px;"
        "  border: 1px solid #cccccc;"
        "  border-radius: 4px;"
        "  padding: 6px;"
        "  background-color: #f9f9f9;"
        "}"
    );
    
    // 初始状态下禁用表单
    enableFormControls(false);
}

void Plan::initDatabase()
{
    // 获取应用程序数据目录 - 使用Qt标准路径获取应用专用数据存储位置
    // 在Linux下通常是 ~/.local/share/<AppName>/
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);  // 创建目录路径（如果不存在）
    
    // 数据库文件路径 - 在应用数据目录下创建plans.db文件
    QString dbPath = dataDir + "/plans.db";
    
    // 创建SQLite数据库连接
    // 使用命名连接"PlanDB"避免与其他数据库连接冲突
    m_database = QSqlDatabase::addDatabase("QSQLITE", "PlanDB");
    m_database.setDatabaseName(dbPath);  // 设置数据库文件路径
    
    // 尝试打开数据库连接
    if (!m_database.open()) {
        // 如果打开失败，显示错误信息并返回
        QMessageBox::critical(this, "数据库错误", 
            QString("无法打开数据库：%1").arg(m_database.lastError().text()));
        return;
    }
    
    // 数据库连接成功后，创建数据表
    createPlanTable();
}

bool Plan::createPlanTable()
{
    // 创建SQL查询对象，绑定到已打开的数据库连接
    QSqlQuery query(m_database);
    
    // 定义创建表的SQL语句 - 使用原始字符串字面量(R"(...)")避免转义字符问题
    QString createTableSql = R"(
        CREATE TABLE IF NOT EXISTS plans (
            id INTEGER PRIMARY KEY AUTOINCREMENT,  -- 主键，自动递增
            name TEXT NOT NULL UNIQUE,             -- 方案名称，非空且唯一
            rtsp_url TEXT NOT NULL,                -- RTSP地址，非空
            ai_enabled INTEGER DEFAULT 0,          -- AI功能开关，0=关闭，1=开启
            region_enabled INTEGER DEFAULT 0,      -- 区域识别开关，0=关闭，1=开启
            object_enabled INTEGER DEFAULT 0,      -- 对象识别开关，0=关闭，1=开启
            object_list TEXT DEFAULT '',           -- 检测对象列表，JSON格式存储
            created_time DATETIME DEFAULT CURRENT_TIMESTAMP,  -- 创建时间，自动设置
            updated_time DATETIME DEFAULT CURRENT_TIMESTAMP   -- 更新时间，自动设置
        )
    )";
    
    // 执行SQL语句创建表
    if (!query.exec(createTableSql)) {
        // 如果创建失败，显示错误信息
        QMessageBox::critical(this, "数据库错误", 
            QString("创建表失败：%1").arg(query.lastError().text()));
        return false;
    }
    
    return true;  // 创建成功
}

bool Plan::loadPlansFromDatabase()
{
    // 清空内存中的方案列表，准备重新加载
    m_plans.clear();
    
    // 创建SQL查询对象
    QSqlQuery query(m_database);
    
    // 执行SELECT语句从数据库查询所有方案数据，按ID排序
    if (!query.exec("SELECT id, name, rtsp_url, ai_enabled, region_enabled, object_enabled, object_list FROM plans ORDER BY id")) {
        // 查询失败时显示警告
        QMessageBox::warning(this, "数据库错误", 
            QString("加载方案失败：%1").arg(query.lastError().text()));
        return false;
    }
    
    // 遍历查询结果，将每行数据转换为PlanData对象
    while (query.next()) {
        PlanData plan;
        // 按字段索引读取数据并转换为对应类型
        plan.id = query.value(0).toInt();              // 第0列：id
        plan.name = query.value(1).toString();         // 第1列：name
        plan.rtspUrl = query.value(2).toString();      // 第2列：rtsp_url
        plan.aiEnabled = query.value(3).toBool();      // 第3列：ai_enabled (转为bool)
        plan.regionEnabled = query.value(4).toBool();  // 第4列：region_enabled (转为bool)
        plan.objectEnabled = query.value(5).toBool();  // 第5列：object_enabled (转为bool)
        // 第6列：object_list (JSON字符串转为QSet<int>)
        plan.objectList = objectListFromJson(query.value(6).toString());
        
        // 将转换后的方案对象添加到内存列表
        m_plans.append(plan);
    }
    
    return true;  // 加载成功
}

bool Plan::savePlanToDatabase(const PlanData& plan)
{
    // 创建SQL查询对象，绑定到数据库连接
    QSqlQuery query(m_database);
    
    // 根据方案ID判断是新增还是更新操作
    if (plan.id == -1) {
        // 新增方案 - 当ID为-1时表示是新方案，需要插入到数据库
        // 使用预处理语句防止SQL注入攻击
        query.prepare(R"(
            INSERT INTO plans (name, rtsp_url, ai_enabled, region_enabled, object_enabled, object_list) 
            VALUES (?, ?, ?, ?, ?, ?)
        )");
        // 按顺序绑定参数值
        query.addBindValue(plan.name);                                    // 方案名称
        query.addBindValue(plan.rtspUrl);                                 // RTSP地址
        query.addBindValue(plan.aiEnabled ? 1 : 0);                      // AI开关（布尔转整数）
        query.addBindValue(plan.regionEnabled ? 1 : 0);                  // 区域识别开关（布尔转整数）
        query.addBindValue(plan.objectEnabled ? 1 : 0);                  // 对象识别开关（布尔转整数）
        query.addBindValue(objectListToJson(plan.objectList));           // 对象列表（转换为JSON字符串）
    } else {
        // 更新现有方案 - 当ID大于0时表示是已存在的方案，需要更新数据库记录
        query.prepare(R"(
            UPDATE plans 
            SET name=?, rtsp_url=?, ai_enabled=?, region_enabled=?, object_enabled=?, object_list=?, updated_time=CURRENT_TIMESTAMP 
            WHERE id=?
        )");
        // 按顺序绑定更新的参数值
        query.addBindValue(plan.name);                                    // 方案名称
        query.addBindValue(plan.rtspUrl);                                 // RTSP地址
        query.addBindValue(plan.aiEnabled ? 1 : 0);                      // AI开关（布尔转整数）
        query.addBindValue(plan.regionEnabled ? 1 : 0);                  // 区域识别开关（布尔转整数）
        query.addBindValue(plan.objectEnabled ? 1 : 0);                  // 对象识别开关（布尔转整数）
        query.addBindValue(objectListToJson(plan.objectList));           // 对象列表（转换为JSON字符串）
        query.addBindValue(plan.id);                                     // WHERE条件：方案ID
    }
    
    // 执行预处理的SQL语句
    if (!query.exec()) {
        // 如果执行失败，显示错误信息并返回false
        QMessageBox::warning(this, "保存错误", 
            QString("保存方案失败：%1").arg(query.lastError().text()));
        return false;
    }
    
    return true;  // 保存成功
}

bool Plan::deletePlanFromDatabase(int planId)
{
    // 创建SQL查询对象，绑定到数据库连接
    QSqlQuery query(m_database);
    
    // 使用预处理语句删除指定ID的方案记录
    // 预处理语句可以防止SQL注入攻击，并提高性能
    query.prepare("DELETE FROM plans WHERE id = ?");
    query.addBindValue(planId);  // 绑定要删除的方案ID
    
    // 执行删除操作
    if (!query.exec()) {
        // 如果删除失败，显示错误信息并返回false
        QMessageBox::warning(this, "删除错误", 
            QString("删除方案失败：%1").arg(query.lastError().text()));
        return false;
    }
    
    return true;  // 删除成功
}

bool Plan::updatePlanInDatabase(const PlanData& plan)
{
    // 更新方案到数据库 - 这是savePlanToDatabase的别名方法
    // 实际上调用savePlanToDatabase来处理更新操作
    // savePlanToDatabase会根据plan.id判断是插入还是更新
    return savePlanToDatabase(plan);
}

void Plan::updatePlanList()
{
    // 清空列表控件中的所有项目，准备重新加载
    m_planListWidget->clear();
    
    // 遍历内存中的方案列表，为每个方案创建列表项
    for (const PlanData& plan : m_plans) {
        // 创建新的列表项，显示方案名称
        QListWidgetItem* item = new QListWidgetItem(plan.name);
        
        // 将方案ID存储在列表项的UserRole数据中，方便后续获取
        // UserRole是Qt预定义的角色，专门用于存储用户自定义数据
        item->setData(Qt::UserRole, plan.id);
        
        // 将列表项添加到列表控件中
        m_planListWidget->addItem(item);
    }
    
    // 如果方案列表不为空，自动选中第一个方案
    // 这样用户打开窗口时就会看到第一个方案的详细信息
    if (!m_plans.isEmpty()) {
        m_planListWidget->setCurrentRow(0);
    }
}

void Plan::updateFormFromPlan(const PlanData& plan)
{
    // 暂时阻止控件的信号发射，避免在更新表单数据时触发不必要的信号
    // 这可以防止循环调用和意外的数据修改标记
    m_nameEdit->blockSignals(true);
    m_rtspEdit->blockSignals(true);
    m_aiCheckBox->blockSignals(true);
    m_regionCheckBox->blockSignals(true);
    m_objectCheckBox->blockSignals(true);
    
    // 从PlanData对象更新表单控件的显示内容
    m_nameEdit->setText(plan.name);                     // 设置方案名称
    m_rtspEdit->setText(plan.rtspUrl);                  // 设置RTSP地址
    m_aiCheckBox->setChecked(plan.aiEnabled);           // 设置AI功能开关状态
    m_regionCheckBox->setChecked(plan.regionEnabled);   // 设置区域识别开关状态
    m_objectCheckBox->setChecked(plan.objectEnabled);   // 设置对象识别开关状态
    
    // 更新对象列表显示 - 将对象ID转换为可读的对象名称
    QStringList objectNames = DetectList::getObjectNames();  // 获取所有可检测对象的名称列表
    QStringList selectedNames;                               // 存储选中对象的名称
    
    // 遍历方案中的对象ID列表，转换为对象名称
    for (int id : plan.objectList) {
        // 检查对象ID是否在有效范围内
        if (id >= 0 && id < objectNames.size()) {
            selectedNames.append(objectNames[id]);  // 添加对应的对象名称
        }
    }
    
    // 将选中的对象名称以逗号分隔的形式显示在文本框中
    m_objectListEdit->setText(selectedNames.join(", "));
    
    // 恢复控件的信号发射功能
    m_nameEdit->blockSignals(false);
    m_rtspEdit->blockSignals(false);
    m_aiCheckBox->blockSignals(false);
    m_regionCheckBox->blockSignals(false);
    m_objectCheckBox->blockSignals(false);
    
    // 重置表单修改标志，表示当前表单内容与数据库一致
    m_formModified = false;
    m_saveButton->setEnabled(false);  // 禁用保存按钮，因为没有修改需要保存
}

void Plan::updatePlanFromForm(PlanData& plan)
{
    // 从表单控件获取用户输入的数据，更新到PlanData对象中
    plan.name = m_nameEdit->text().trimmed();              // 获取方案名称并去除首尾空格
    plan.rtspUrl = m_rtspEdit->text().trimmed();           // 获取RTSP地址并去除首尾空格
    plan.aiEnabled = m_aiCheckBox->isChecked();            // 获取AI功能开关状态
    plan.regionEnabled = m_regionCheckBox->isChecked();    // 获取区域识别开关状态
    plan.objectEnabled = m_objectCheckBox->isChecked();    // 获取对象识别开关状态
    
    // 注意：objectList字段在用户选择对象时已经通过onSelectObjects函数更新
    // 这里不需要再次处理，因为对象列表的更新是通过专门的对象选择对话框完成的
}

void Plan::clearForm()
{
    m_nameEdit->clear();
    m_rtspEdit->clear();
    m_aiCheckBox->setChecked(false);
    m_regionCheckBox->setChecked(false);
    m_objectCheckBox->setChecked(false);
    m_objectListEdit->clear();
    
    m_formModified = false;
    m_saveButton->setEnabled(false);
}

void Plan::enableFormControls(bool enabled)
{
    m_nameEdit->setEnabled(enabled);
    m_rtspEdit->setEnabled(enabled);
    m_aiCheckBox->setEnabled(enabled);
    m_regionCheckBox->setEnabled(enabled);
    m_objectCheckBox->setEnabled(enabled);
    m_selectObjectButton->setEnabled(enabled);
    m_applyButton->setEnabled(enabled);
}

QString Plan::objectListToJson(const QSet<int>& objectList)
{
    // 将QSet<int>类型的对象ID集合转换为JSON字符串格式
    // 这样可以在SQLite数据库中以TEXT字段存储对象列表
    
    QJsonArray jsonArray;  // 创建JSON数组对象
    
    // 遍历对象ID集合，将每个ID添加到JSON数组中
    for (int id : objectList) {
        jsonArray.append(id);  // 将整数ID添加到JSON数组
    }
    
    // 将JSON数组转换为JSON文档，然后转换为紧凑格式的字符串
    // Compact格式去除了不必要的空格，减少存储空间
    QJsonDocument doc(jsonArray);
    return doc.toJson(QJsonDocument::Compact);
}

QSet<int> Plan::objectListFromJson(const QString& jsonString)
{
    // 将JSON字符串格式的对象列表转换回QSet<int>类型
    // 用于从SQLite数据库中读取对象列表数据
    
    QSet<int> objectList;  // 创建空的对象ID集合
    
    // 如果JSON字符串为空，直接返回空集合
    if (jsonString.isEmpty()) {
        return objectList;
    }
    
    // 尝试解析JSON字符串
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    
    // 检查解析后的JSON文档是否为数组格式
    if (doc.isArray()) {
        QJsonArray jsonArray = doc.array();  // 获取JSON数组
        
        // 遍历JSON数组中的每个值
        for (const QJsonValue& value : jsonArray) {
            // 检查值是否为数字类型（JSON中的数字都是double类型）
            if (value.isDouble()) {
                // 将数字值转换为整数并添加到集合中
                objectList.insert(value.toInt());
            }
        }
    }
    
    return objectList;  // 返回转换后的对象ID集合
}

void Plan::createDefaultPlans()
{
    // 创建三个默认方案
    QList<PlanData> defaultPlans;
    
    // 方案1：基础监控
    PlanData plan1;
    plan1.name = "基础监控方案";
    plan1.rtspUrl = "rtsp://192.168.1.130/live/0";
    plan1.aiEnabled = true;
    plan1.regionEnabled = false;
    plan1.objectEnabled = true;
    plan1.objectList = {0, 15}; // 人和猫（示例）
    
    // 方案2：区域监控
    PlanData plan2;
    plan2.name = "区域监控方案";
    plan2.rtspUrl = "rtsp://192.168.1.131/live/0";
    plan2.aiEnabled = true;
    plan2.regionEnabled = true;
    plan2.objectEnabled = true;
    plan2.objectList = {0, 1, 2}; // 人、自行车、汽车
    
    // 方案3：全功能监控
    PlanData plan3;
    plan3.name = "全功能监控方案";
    plan3.rtspUrl = "rtsp://192.168.1.132/live/0";
    plan3.aiEnabled = true;
    plan3.regionEnabled = true;
    plan3.objectEnabled = true;
    plan3.objectList = {0, 1, 2, 3, 5, 7}; // 多种对象
    
    defaultPlans << plan1 << plan2 << plan3;
    
    // 保存到数据库
    for (const PlanData& plan : defaultPlans) {
        if (savePlanToDatabase(plan)) {
            // 重新加载以获取数据库生成的ID
            loadPlansFromDatabase();
        }
    }
}

// 当用户在左侧方案列表中选择不同方案时触发
void Plan::onPlanListSelectionChanged()
{
    // 获取当前选中的行号
    int currentRow = m_planListWidget->currentRow();
    
    // 检查选中的行号是否有效
    if (currentRow >= 0 && currentRow < m_plans.size()) {
        // 有效选择：更新当前方案索引
        m_currentPlanIndex = currentRow;
        // 将选中方案的数据填充到右侧表单中
        updateFormFromPlan(m_plans[currentRow]);
        // 启用右侧的表单控件，允许用户编辑
        enableFormControls(true);
        // 启用删除按钮，允许删除当前选中的方案
        m_deleteButton->setEnabled(true);
    } else {
        // 无效选择或未选择：重置状态
        m_currentPlanIndex = -1;
        // 清空右侧表单内容
        clearForm();
        // 禁用右侧的表单控件，防止误操作
        enableFormControls(false);
        // 禁用删除按钮，因为没有选中任何方案
        m_deleteButton->setEnabled(false);
    }
}

void Plan::onNewPlan()
{
    // 创建新的方案对象
    PlanData newPlan;
    
    // 为新方案设置默认值
    newPlan.name = QString("新方案 %1").arg(m_plans.size() + 1);    // 自动生成方案名称
    newPlan.rtspUrl = "rtsp://192.168.1.130/live/0";               // 设置默认RTSP地址
    newPlan.aiEnabled = false;                                     // 默认关闭AI功能
    newPlan.regionEnabled = false;                                 // 默认关闭区域识别
    newPlan.objectEnabled = false;                                 // 默认关闭对象识别
    newPlan.objectList.clear();                                    // 清空对象列表
    
    // 将新方案添加到内存中的方案列表
    m_plans.append(newPlan);
    
    // 更新左侧的方案列表显示
    updatePlanList();
    
    // 自动选择新创建的方案（最后一个）
    m_planListWidget->setCurrentRow(m_plans.size() - 1);
    
    // 设置表单修改标志，使保存按钮变为可用状态
    // 新创建的方案需要保存才能持久化到数据库
    m_formModified = true;
    m_saveButton->setEnabled(true);
    
    // 自动选中方案名称文本框内容，方便用户直接输入新名称
    m_nameEdit->selectAll();
    m_nameEdit->setFocus();  // 将焦点设置到名称编辑框
}

void Plan::onSavePlan()
{
    // 检查当前是否有有效的方案被选中
    if (m_currentPlanIndex < 0 || m_currentPlanIndex >= m_plans.size()) {
        return;  // 没有选中的方案，直接返回
    }
    
    // 从表单控件获取用户输入的数据进行验证
    QString name = m_nameEdit->text().trimmed();      // 获取方案名称并去除首尾空格
    QString rtspUrl = m_rtspEdit->text().trimmed();   // 获取RTSP地址并去除首尾空格
    
    // 验证方案名称是否为空
    if (name.isEmpty()) {
        QMessageBox::warning(this, "保存错误", "方案名称不能为空！");
        m_nameEdit->setFocus();  // 将焦点设置到名称编辑框
        return;
    }
    
    // 验证RTSP地址是否为空
    if (rtspUrl.isEmpty()) {
        QMessageBox::warning(this, "保存错误", "RTSP地址不能为空！");
        m_rtspEdit->setFocus();  // 将焦点设置到RTSP地址编辑框
        return;
    }
    
    // 检查方案名称是否与其他方案重复（排除当前正在编辑的方案）
    for (int i = 0; i < m_plans.size(); ++i) {
        if (i != m_currentPlanIndex && m_plans[i].name == name) {
            QMessageBox::warning(this, "保存错误", "方案名称已存在，请使用其他名称！");
            m_nameEdit->setFocus();     // 将焦点设置到名称编辑框
            m_nameEdit->selectAll();    // 选中所有文本，方便用户重新输入
            return;
        }
    }
    
    // 数据验证通过，将表单数据更新到方案对象中
    updatePlanFromForm(m_plans[m_currentPlanIndex]);
    
    // 尝试将方案保存到数据库
    if (savePlanToDatabase(m_plans[m_currentPlanIndex])) {
        // 保存成功后的处理
        
        // 如果是新方案（ID为-1），需要重新从数据库加载以获取自动生成的ID
        if (m_plans[m_currentPlanIndex].id == -1) {
            int currentPlanIndex = m_currentPlanIndex;   // 保存当前选中的索引
            loadPlansFromDatabase();                     // 重新从数据库加载所有方案
            updatePlanList();                            // 更新方案列表显示
            m_planListWidget->setCurrentRow(currentPlanIndex);  // 恢复选中状态
        } else {
            // 如果是更新现有方案，只需要更新列表项的显示名称
            QListWidgetItem* item = m_planListWidget->item(m_currentPlanIndex);
            if (item) {
                item->setText(m_plans[m_currentPlanIndex].name);  // 更新显示的方案名称
            }
        }
        
        // 显示保存成功的提示信息
        QMessageBox::information(this, "保存成功", "方案已成功保存！");
        
        // 重置表单修改标志和保存按钮状态
        m_formModified = false;
        m_saveButton->setEnabled(false);
    }
    // 如果保存失败，savePlanToDatabase函数内部已经显示了错误信息
}

void Plan::onDeletePlan()
{
    // 检查当前是否有有效的方案被选中
    if (m_currentPlanIndex < 0 || m_currentPlanIndex >= m_plans.size()) {
        return;  // 没有选中的方案，直接返回
    }
    
    // 获取当前选中的方案对象
    const PlanData& plan = m_plans[m_currentPlanIndex];
    
    // 显示确认删除对话框，防止误操作
    int ret = QMessageBox::question(this, "确认删除", 
        QString("确定要删除方案 \"%1\" 吗？\n此操作无法撤销。").arg(plan.name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);  // 默认选择"否"
    
    // 用户确认删除操作
    if (ret == QMessageBox::Yes) {
        // 如果方案已经保存到数据库（ID不为-1），需要从数据库中删除
        if (plan.id != -1) {
            deletePlanFromDatabase(plan.id);  // 从数据库删除记录
        }
        
        // 从内存中的方案列表中移除该方案
        m_plans.removeAt(m_currentPlanIndex);
        
        // 更新界面显示
        updatePlanList();  // 重新加载方案列表显示
        
        // 重置当前选中的方案索引
        m_currentPlanIndex = -1;
        
        // 清空右侧的表单内容
        clearForm();
        
        // 禁用表单控件和相关按钮
        enableFormControls(false);
        m_deleteButton->setEnabled(false);
        
        // 显示删除成功的提示信息
        QMessageBox::information(this, "删除成功", "方案已成功删除！");
    }
}

void Plan::onApplyPlan()
{
    // 检查当前是否有有效的方案被选中
    if (m_currentPlanIndex < 0 || m_currentPlanIndex >= m_plans.size()) {
        return;  // 没有选中的方案，直接返回
    }
    
    // 获取当前选中的方案对象
    const PlanData& plan = m_plans[m_currentPlanIndex];
    
    // 显示应用方案的确认对话框，详细列出将要执行的配置
    int ret = QMessageBox::question(this, "应用方案", 
        QString("确定要应用方案 \"%1\" 吗？\n这将会：\n"
               "• 设置RTSP地址：%2\n"
               "• 配置AI功能：%3\n"
               "• 配置区域识别：%4\n"
               "• 配置对象识别：%5\n"
               "• 设置检测对象列表")
        .arg(plan.name)                                        // 方案名称
        .arg(plan.rtspUrl)                                     // RTSP地址
        .arg(plan.aiEnabled ? "启用" : "禁用")                  // AI功能状态
        .arg(plan.regionEnabled ? "启用" : "禁用")              // 区域识别状态
        .arg(plan.objectEnabled ? "启用" : "禁用"),             // 对象识别状态
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);  // 默认选择"是"
    
    // 用户确认应用方案
    if (ret == QMessageBox::Yes) {
        // 通过信号将方案数据发送给Controller
        // Controller会根据方案数据同步主界面的各种设置
        emit planApplied(plan);
        
        // 显示应用成功的提示信息
        QMessageBox::information(this, "应用成功", 
            QString("方案 \"%1\" 配置已发送给系统！").arg(plan.name));
        
        // 关闭方案管理窗口，返回主界面
        accept();  // 相当于点击了"确定"按钮，会触发对话框的accepted()信号
    }
}

void Plan::onFormDataChanged()
{
    // 当表单中的任何数据发生变化时调用此函数
    // 这个槽函数连接到表单中各个控件的信号（如textChanged、toggled等）
    
    m_formModified = true;          // 设置表单修改标志为true
    m_saveButton->setEnabled(true); // 启用保存按钮，提示用户有未保存的修改
}
