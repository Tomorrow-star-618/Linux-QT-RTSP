#ifndef PLAN_H
#define PLAN_H

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSplitter>
#include <QGroupBox>
#include <QComboBox>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSet>
#include <QMessageBox>

// 方案数据结构
struct PlanData {
    int id;                    // 方案ID（数据库主键）
    int cameraId;              // 摄像头ID（0表示主流，1-N表示子流）
    QString name;              // 方案名称
    QString rtspUrl;           // RTSP地址
    bool aiEnabled;            // AI识别功能使能
    bool regionEnabled;        // 区域识别功能使能
    bool objectEnabled;        // 对象识别功能使能
    QSet<int> objectList;      // 对象列表（对象ID集合）
    
    // 默认构造函数
    PlanData() : id(-1), cameraId(0), aiEnabled(false), regionEnabled(false), objectEnabled(false) {}
};

class Plan : public QDialog
{
    Q_OBJECT

public:
    explicit Plan(QWidget *parent = nullptr);
    ~Plan();

signals:
    // 应用方案信号，发送方案配置信息给Controller
    void planApplied(const PlanData& plan);

private slots:
    void onPlanListSelectionChanged();  // 处理方案列表选择变化事件
    void onNewPlan();                   // 处理新建方案按钮点击事件
    void onSavePlan();                  // 处理保存方案按钮点击事件
    void onDeletePlan();                // 处理删除方案按钮点击事件
    void onApplyPlan();                 // 处理应用方案按钮点击事件
    void onFormDataChanged();           // 处理表单数据变化事件（启用保存按钮）

private:
    // 界面初始化函数
    void initUI();          // 初始化用户界面，创建并布局所有控件
    void initDatabase();    // 初始化数据库连接，创建数据库文件和表结构
    
    // 数据库操作函数
    bool createPlanTable();                         // 创建方案数据表，定义表结构和字段
    bool loadPlansFromDatabase();                   // 从数据库加载所有方案数据到内存
    bool savePlanToDatabase(const PlanData& plan);  // 保存方案数据到数据库（新增或更新）
    bool deletePlanFromDatabase(int planId);        // 根据ID从数据库删除指定方案
    bool updatePlanInDatabase(const PlanData& plan); // 更新数据库中现有方案的信息
    
    // 界面操作函数
    void updatePlanList();                          // 更新左侧方案列表显示
    void updateFormFromPlan(const PlanData& plan);  // 将方案数据填充到右侧表单控件
    void updatePlanFromForm(PlanData& plan);        // 从表单控件获取更新方案对象的数据
    void clearForm();                               // 清空右侧表单的所有内容
    void enableFormControls(bool enabled);          // 启用或禁用右侧表单控件的编辑功能
    
    // 对象列表序列化转换函数
    QString objectListToJson(const QSet<int>& objectList);  // 将对象ID集合转换为JSON字符串格式
    QSet<int> objectListFromJson(const QString& jsonString); // 将JSON字符串解析为对象ID集合
    
    // 界面控件
    QSplitter* m_splitter;
    
    // 左侧方案列表
    QListWidget* m_planListWidget;
    QPushButton* m_newButton;
    QPushButton* m_deleteButton;
    
    // 右侧方案配置
    QComboBox* m_cameraIdComboBox;  // 摄像头ID选择（0=主流，1-N=子流）
    QLineEdit* m_nameEdit;          // 方案名称
    QLineEdit* m_rtspEdit;          // RTSP地址
    QCheckBox* m_aiCheckBox;        // AI功能
    QCheckBox* m_regionCheckBox;    // 区域识别
    QCheckBox* m_objectCheckBox;    // 对象识别
    QTextEdit* m_objectListEdit;    // 对象列表（显示为文本）
    QPushButton* m_selectObjectButton; // 选择对象按钮
    QPushButton* m_saveButton;      // 保存按钮
    QPushButton* m_applyButton;     // 应用按钮
    
    // 数据
    QSqlDatabase m_database;
    QList<PlanData> m_plans;        // 方案列表
    int m_currentPlanIndex;         // 当前选中的方案索引
    bool m_formModified;            // 表单是否已修改
    
    // 默认方案创建函数
    void createDefaultPlans();  // 创建系统默认的三个示例方案（基础监控、区域监控、全功能监控）
};

#endif // PLAN_H
