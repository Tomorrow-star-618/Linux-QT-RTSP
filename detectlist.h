#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QString>
#include <QSet>
#include <QLineEdit> // Added for search input

class DetectList : public QWidget {
    Q_OBJECT

public:   
    explicit DetectList(QWidget* parent = nullptr);  
    ~DetectList();

    QSet<int> getSelectedObjects() const;                          // 获取当前选中的对象ID集合
    void setSelectedObjects(const QSet<int>& selectedIds);         // 设置选中的对象ID集合
    static QStringList getObjectNames();                           // 获取所有对象名称（COCO数据集80类）

signals:
    void selectionChanged(const QSet<int>& selectedIds);           // 当选中对象发生变化时发出信号

private slots:
    void onSelectAllClicked();         // “全选”按钮点击槽函数
    void onClearAllClicked();          // “清空”按钮点击槽函数
    void onApplyClicked();             // “应用”按钮点击槽函数
    void onCancelClicked();            // “取消”按钮点击槽函数
    void onCheckBoxChanged();          // 复选框状态变化槽函数
    void onSearchTextChanged(const QString& text); // 搜索框文本变化槽函数

private:
    void initUI();                     // 初始化界面UI
    void createObjectCheckBoxes();     // 创建对象复选框
    void updateSelectionCount();       // 更新已选中数量显示
    void updateSearchHighlight(const QString& text); // 更新搜索高亮

    QScrollArea* m_scrollArea;         // 滚动区域
    QWidget* m_contentWidget;          // 滚动区域内容部件
    QGridLayout* m_gridLayout;         // 网格布局
    QList<QCheckBox*> m_checkBoxes;    // 所有对象复选框列表
    QLabel* m_countLabel;              // 显示已选中数量的标签
    QPushButton* m_selectAllBtn;       // “全选”按钮
    QPushButton* m_clearAllBtn;        // “清空”按钮
    QPushButton* m_applyBtn;           // “应用”按钮
    QPushButton* m_cancelBtn;          // “取消”按钮
    QLineEdit* m_searchEdit = nullptr; // 搜索输入框
    QSet<int> m_selectedIds;           // 当前选中的对象ID集合
    
    // COCO数据集80个对象名称静态常量
    static const QStringList s_objectNames;
}; 