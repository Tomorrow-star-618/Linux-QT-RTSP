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

class DetectList : public QWidget {
    Q_OBJECT

public:
    explicit DetectList(QWidget* parent = nullptr);
    ~DetectList();

    // 获取选中的对象列表
    QSet<int> getSelectedObjects() const;
    
    // 设置选中的对象
    void setSelectedObjects(const QSet<int>& selectedIds);
    
    // 获取所有对象名称
    static QStringList getObjectNames();

signals:
    void selectionChanged(const QSet<int>& selectedIds);

private slots:
    void onSelectAllClicked();
    void onClearAllClicked();
    void onApplyClicked();
    void onCancelClicked();
    void onCheckBoxChanged();

private:
    void initUI();
    void createObjectCheckBoxes();
    void updateSelectionCount();

    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QGridLayout* m_gridLayout;
    QList<QCheckBox*> m_checkBoxes;
    QLabel* m_countLabel;
    QPushButton* m_selectAllBtn;
    QPushButton* m_clearAllBtn;
    QPushButton* m_applyBtn;
    QPushButton* m_cancelBtn;
    
    QSet<int> m_selectedIds;
    
    // COCO数据集80个对象名称
    static const QStringList s_objectNames;
}; 