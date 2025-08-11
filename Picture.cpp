#include "Picture.h"
#include <QHBoxLayout>
#include <QDir>
#include <QFileInfoList>
#include <QPixmap>
#include <QMessageBox>
#include <QCoreApplication>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QWheelEvent>

Picture::Picture(QWidget* parent)
    : QWidget(parent), currentIndex(0)
{
    this->setWindowTitle("电子相册 - 截图相册");
    this->resize(900, 600);
    // 设置窗口背景为渐变色
    this->setStyleSheet(
        "QWidget { "
        "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, "
        "    stop: 0 #f8f9fa, stop: 1 #e9ecef); "
        "  color: #333; "
        "}"
    );

    // 创建控件
    prevBtn = new QPushButton("上一张", this);
    nextBtn = new QPushButton("下一张", this);
    closeBtn = new QPushButton("关闭", this); // 关闭按钮
    screenshotAlbumBtn = new QPushButton("截图相册", this);
    alarmAlbumBtn = new QPushButton("报警相册", this);
    
    // 设置按钮样式
    prevBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #ffffff;"
        "  border: 2px solid #dee2e6;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  padding: 8px 16px;"
        "  color: #495057;"
        "}"
        "QPushButton:hover {"
        "  background-color: #e9ecef;"
        "  border-color: #adb5bd;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #dee2e6;"
        "}"
    );
    nextBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #ffffff;"
        "  border: 2px solid #dee2e6;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  padding: 8px 16px;"
        "  color: #495057;"
        "}"
        "QPushButton:hover {"
        "  background-color: #e9ecef;"
        "  border-color: #adb5bd;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #dee2e6;"
        "}"
    );
    closeBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #dc3545;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "  padding: 6px 12px;"
        "  color: white;"
        "}"
        "QPushButton:hover {"
        "  background-color: #c82333;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #bd2130;"
        "}"
    );
    // 截图相册按钮默认选中（橙色），报警相册按钮默认未选中（绿色）
    screenshotAlbumBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #fd7e14;"
        "  border: 2px solid #fd7e14;"
        "  border-radius: 10px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  padding: 8px 16px;"
        "  color: white;"
        "}"
        "QPushButton:hover {"
        "  background-color: #e8660c;"
        "  border-color: #e8660c;"
        "}"
    );
    alarmAlbumBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #ffffff;"
        "  border: 2px solid #28a745;"
        "  border-radius: 10px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  padding: 8px 16px;"
        "  color: #28a745;"
        "}"
        "QPushButton:hover {"
        "  background-color: #f8f9fa;"
        "  border-color: #1e7e34;"
        "  color: #1e7e34;"
        "}"
    );
    
    // 设置按钮大小
    prevBtn->setFixedSize(80, 40);
    nextBtn->setFixedSize(80, 40);
    closeBtn->setFixedSize(60, 30);
    screenshotAlbumBtn->setFixedSize(100, 35);
    alarmAlbumBtn->setFixedSize(100, 35);
    
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(500, 400);
    imageLabel->setStyleSheet(
        "QLabel {"
        "  background-color: #ffffff;"
        "  border: 2px solid #dee2e6;"
        "  border-radius: 12px;"
        "  padding: 20px;"
        "  margin: 10px;"
        "  box-shadow: 0 4px 8px rgba(0,0,0,0.1);"
        "}"
    );

    // 上方按钮布局
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->addWidget(screenshotAlbumBtn);
    topLayout->addWidget(alarmAlbumBtn);
    topLayout->addStretch(); // 添加弹性空间，让按钮靠左
    topLayout->addWidget(closeBtn); // 关闭按钮放在右侧

    // 中间区域布局 - 左边按钮、图片区域、右边按钮
    QHBoxLayout* middleLayout = new QHBoxLayout();
    
    // 左侧控制区域
    QVBoxLayout* leftControlLayout = new QVBoxLayout();
    leftControlLayout->addStretch();
    leftControlLayout->addWidget(prevBtn);
    leftControlLayout->addStretch();
    
    // 右侧控制区域
    QVBoxLayout* rightControlLayout = new QVBoxLayout();
    rightControlLayout->addStretch();
    rightControlLayout->addWidget(nextBtn);
    rightControlLayout->addStretch();
    
    middleLayout->addLayout(leftControlLayout, 0);
    middleLayout->addWidget(imageLabel, 1); // 图片区域占主要空间
    middleLayout->addLayout(rightControlLayout, 0);

    // 主布局 - 垂直排列
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(middleLayout, 1); // 中间区域占主要空间
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 连接信号槽
    connect(prevBtn, &QPushButton::clicked, this, &Picture::onPrevClicked);
    connect(nextBtn, &QPushButton::clicked, this, &Picture::onNextClicked);
    connect(closeBtn, &QPushButton::clicked, this, &Picture::onCloseClicked);
    connect(screenshotAlbumBtn, &QPushButton::clicked, this, &Picture::onScreenshotAlbumClicked);
    connect(alarmAlbumBtn, &QPushButton::clicked, this, &Picture::onAlarmAlbumClicked);

    // 加载图片
    loadImages();
    updateImage();
}

Picture::~Picture() {}

void Picture::loadImages()
{
    imageFiles.clear();
    // 确保picture文件夹存在（使用源码路径）
    QString sourcePath = QString(__FILE__).section('/', 0, -2); // 获取源码目录路径
    QString albumPath;
    
    if (isScreenshotAlbum) {
        albumPath = sourcePath + "/picture/save-picture";
    } else {
        albumPath = sourcePath + "/picture/alarm-picture";
    }
    
    QDir dir(albumPath);
    // 如果目录不存在，创建它
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QStringList filters;
    filters << "*.jpg" << "*.png" << "*.jpeg" << "*.bmp";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::NoSymLinks);
    for (const QFileInfo& fileInfo : fileList) {
        imageFiles << fileInfo.absoluteFilePath();
    }
    currentIndex = 0;
}

void Picture::updateImage()
{
    if (imageFiles.isEmpty()) {
        imageLabel->setText("没有图片");
        prevBtn->setEnabled(false);
        nextBtn->setEnabled(false);
        return;
    }
    QPixmap pix(imageFiles[currentIndex]);
    if (pix.isNull()) {
        imageLabel->setText("图片加载失败");
    } else {
        QSize targetSize = imageLabel->size() * scaleFactor;
        imageLabel->setPixmap(pix.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    }
    prevBtn->setEnabled(currentIndex > 0);
    nextBtn->setEnabled(currentIndex < imageFiles.size() - 1);
}

void Picture::onPrevClicked()
{
    if (currentIndex > 0) {
        --currentIndex;
        updateImage();
    }
}

void Picture::onNextClicked()
{
    if (currentIndex < imageFiles.size() - 1) {
        ++currentIndex;
        updateImage();
    }
}

void Picture::onCloseClicked()
{
    this->close(); // 关闭窗口，自动释放
}

void Picture::onScreenshotAlbumClicked()
{
    if (!isScreenshotAlbum) {
        // 切换到截图相册模式
        isScreenshotAlbum = true;
        
        // 更新按钮样式 - 截图相册选中（橙色），报警相册未选中（白色边框绿色）
        screenshotAlbumBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #fd7e14;"
            "  border: 2px solid #fd7e14;"
            "  border-radius: 10px;"
            "  font-size: 14px;"
            "  font-weight: bold;"
            "  padding: 8px 16px;"
            "  color: white;"
            "}"
            "QPushButton:hover {"
            "  background-color: #e8660c;"
            "  border-color: #e8660c;"
            "}"
        );
        alarmAlbumBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #ffffff;"
            "  border: 2px solid #28a745;"
            "  border-radius: 10px;"
            "  font-size: 14px;"
            "  font-weight: bold;"
            "  padding: 8px 16px;"
            "  color: #28a745;"
            "}"
            "QPushButton:hover {"
            "  background-color: #f8f9fa;"
            "  border-color: #1e7e34;"
            "  color: #1e7e34;"
            "}"
        );
        
        // 重新加载截图相册的图片
        loadImages();
        updateImage();
        
        // 更新窗口标题
        this->setWindowTitle("电子相册 - 截图相册");
    }
}

void Picture::onAlarmAlbumClicked()
{
    if (isScreenshotAlbum) {
        // 切换到报警相册模式
        isScreenshotAlbum = false;
        
        // 更新按钮样式 - 报警相册选中（绿色），截图相册未选中（白色边框橙色）
        screenshotAlbumBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #ffffff;"
            "  border: 2px solid #fd7e14;"
            "  border-radius: 10px;"
            "  font-size: 14px;"
            "  font-weight: bold;"
            "  padding: 8px 16px;"
            "  color: #fd7e14;"
            "}"
            "QPushButton:hover {"
            "  background-color: #f8f9fa;"
            "  border-color: #e8660c;"
            "  color: #e8660c;"
            "}"
        );
        alarmAlbumBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #28a745;"
            "  border: 2px solid #28a745;"
            "  border-radius: 10px;"
            "  font-size: 14px;"
            "  font-weight: bold;"
            "  padding: 8px 16px;"
            "  color: white;"
            "}"
            "QPushButton:hover {"
            "  background-color: #1e7e34;"
            "  border-color: #1e7e34;"
            "}"
        );
        
        // 重新加载报警相册的图片
        loadImages();
        updateImage();
        
        // 更新窗口标题
        this->setWindowTitle("电子相册 - 报警相册");
    }
}

void Picture::wheelEvent(QWheelEvent* event)
{
    if (imageFiles.isEmpty()) return;
    if (event->angleDelta().y() > 0) {
        // 向上滚动，放大
        if (scaleFactor < 3.0) {
            scaleFactor += 0.1;
            updateImage();
        }
    } else if (event->angleDelta().y() < 0) {
        // 向下滚动，缩小
        if (scaleFactor > 0.2) {
            scaleFactor -= 0.1;
            updateImage();
        }
    }
    event->accept();
}
