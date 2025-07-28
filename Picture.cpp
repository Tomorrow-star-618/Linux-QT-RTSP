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
    this->setWindowTitle("电子相册");
    this->resize(800, 480);
    this->setStyleSheet("background:transparent;color:black;border:none;padding:0px;margin:0px;"); // 无边框、无内外边距
    // 设置窗口背景为白色
    this->setStyleSheet("background-color: white;");

    // 创建控件
    prevBtn = new QPushButton("上一张", this);
    nextBtn = new QPushButton("下一张", this);
    closeBtn = new QPushButton("关闭", this); // 右上角关闭按钮
    zoomInBtn = new QPushButton("放大", this); // 放大按钮
    zoomOutBtn = new QPushButton("缩小", this); // 缩小按钮
    // 设置按钮背景为白色
    prevBtn->setStyleSheet("background-color: white;");
    nextBtn->setStyleSheet("background-color: white;");
    closeBtn->setStyleSheet("background-color: white;");
    zoomInBtn->setStyleSheet("background-color: white;");
    zoomOutBtn->setStyleSheet("background-color: white;");
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(400, 300);
    imageLabel->setStyleSheet("background:transparent;color:black;border:none;padding:0px;margin:0px;"); // 无边框、无内外边距

    // 右边垂直布局
    QWidget* rightplane = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightplane);
    rightLayout->addWidget(closeBtn);
    rightLayout->addWidget(prevBtn);
    rightLayout->addWidget(nextBtn);
    rightLayout->addWidget(zoomInBtn);
    rightLayout->addWidget(zoomOutBtn);

    // 主水平布局
    QHBoxLayout* hLayout = new QHBoxLayout(this);
    hLayout->addWidget(imageLabel,3);
    hLayout->addWidget(rightplane,1);

    // 连接信号槽
    connect(prevBtn, &QPushButton::clicked, this, &Picture::onPrevClicked);
    connect(nextBtn, &QPushButton::clicked, this, &Picture::onNextClicked);
    connect(closeBtn, &QPushButton::clicked, this, &Picture::onCloseClicked);
    connect(zoomInBtn, &QPushButton::clicked, this, &Picture::onZoomInClicked);
    connect(zoomOutBtn, &QPushButton::clicked, this, &Picture::onZoomOutClicked);

    // 加载图片
    loadImages();
    updateImage();
}

Picture::~Picture() {}

void Picture::loadImages()
{
    imageFiles.clear();
    QDir dir(QCoreApplication::applicationDirPath() + "/picture");
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
        zoomInBtn->setEnabled(false);
        zoomOutBtn->setEnabled(false);
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
    zoomInBtn->setEnabled(scaleFactor < 3.0);
    zoomOutBtn->setEnabled(scaleFactor > 0.2);
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

void Picture::onZoomInClicked()
{
    if (scaleFactor < 3.0) {
        scaleFactor += 0.1;
        updateImage();
    }
}

void Picture::onZoomOutClicked()
{
    if (scaleFactor > 0.2) {
        scaleFactor -= 0.1;
        updateImage();
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
 