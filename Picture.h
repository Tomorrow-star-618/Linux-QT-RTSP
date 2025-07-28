#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QStringList>

class Picture : public QWidget {
    Q_OBJECT
public:
    explicit Picture(QWidget* parent = nullptr);
    ~Picture();

private slots:
    void onPrevClicked(); // 上一张
    void onNextClicked(); // 下一张

private:
    void loadImages();    // 加载picture文件夹下所有图片
    void updateImage();   // 更新当前显示图片

    QLabel* imageLabel;   // 图片显示区域
    QPushButton* prevBtn; // 左侧按钮
    QPushButton* nextBtn; // 右侧按钮
    QStringList imageFiles; // 图片文件路径列表
    int currentIndex;     // 当前图片索引
}; 