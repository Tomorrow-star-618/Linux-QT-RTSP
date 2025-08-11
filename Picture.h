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
    void onCloseClicked(); // 关闭窗口
    void onScreenshotAlbumClicked(); // 截图相册
    void onAlarmAlbumClicked(); // 报警相册

private:
    void loadImages();    // 加载picture文件夹下所有图片
    void updateImage();   // 更新当前显示图片

    QLabel* imageLabel;   // 图片显示区域
    QPushButton* prevBtn; // 左侧按钮
    QPushButton* nextBtn; // 右侧按钮
    QPushButton* closeBtn; // 关闭按钮
    QPushButton* screenshotAlbumBtn; // 截图相册按钮
    QPushButton* alarmAlbumBtn;      // 报警相册按钮
    QStringList imageFiles; // 图片文件路径列表
    int currentIndex;     // 当前图片索引
    double scaleFactor = 1.0; // 当前缩放比例
    bool isScreenshotAlbum = true; // 当前是否为截图相册模式，默认true

protected:
    void wheelEvent(QWheelEvent* event) override; // 鼠标滚轮事件处理
};