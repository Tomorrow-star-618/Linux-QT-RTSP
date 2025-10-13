#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QStringList>
#include <QSlider>
#include <QLineEdit>
#include <QComboBox>

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
    void onSliderValueChanged(int value); // 滑动条值改变
    void onJumpToImage(); // 跳转到指定图片
    void onDeleteImage(); // 删除当前图片
    void onSortOrderChanged(); // 排序方式改变
    void onCameraFilterChanged(int index); // 摄像头筛选改变

private:
    void loadImages();    // 加载picture文件夹下所有图片
    void updateImage();   // 更新当前显示图片
    void updateImageInfo(); // 更新图片信息显示
    void sortImages();    // 对图片按时间排序
    QString extractTimeFromFilename(const QString& filename); // 从文件名提取时间
    int extractCameraIdFromFilename(const QString& filename); // 从文件名提取摄像头ID
    void updateCameraFilter(); // 更新摄像头筛选下拉框

    QLabel* imageLabel;   // 图片显示区域
    QPushButton* prevBtn; // 左侧按钮
    QPushButton* nextBtn; // 右侧按钮
    QPushButton* closeBtn; // 关闭按钮
    QPushButton* screenshotAlbumBtn; // 截图相册按钮
    QPushButton* alarmAlbumBtn;      // 报警相册按钮
    QPushButton* deleteBtn;          // 删除按钮
    QSlider* imageSlider;            // 图片导航滑动条
    QLineEdit* jumpEdit;             // 跳转输入框
    QPushButton* jumpBtn;            // 跳转按钮
    QComboBox* sortComboBox;         // 排序方式下拉框
    QComboBox* cameraFilterComboBox; // 摄像头筛选下拉框
    QLabel* infoLabel;               // 图片信息标签(总数/序号)
    QLabel* timeLabel;               // 时间信息标签
    QStringList imageFiles; // 图片文件路径列表（筛选后的）
    QStringList allImageFiles; // 所有图片文件路径列表（未筛选）
    int currentIndex;     // 当前图片索引
    double scaleFactor = 1.0; // 当前缩放比例
    bool isScreenshotAlbum = true; // 当前是否为截图相册模式，默认true
    bool isAscendingOrder = false; // 是否按时间升序排列，默认false(降序)
    int currentCameraFilter = -1; // 当前筛选的摄像头ID，-1表示显示所有

protected:
    void wheelEvent(QWheelEvent* event) override; // 鼠标滚轮事件处理
};