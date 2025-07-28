#include "Picture.h"
#include <QHBoxLayout>
#include <QDir>
#include <QFileInfoList>
#include <QPixmap>
#include <QMessageBox>
#include <QCoreApplication>
#include <QPushButton>
#include <QLabel>

Picture::Picture(QWidget* parent)
    : QWidget(parent), currentIndex(0)
{
    this->setWindowTitle("电子相册");
    this->resize(600, 400);

    // 创建控件
    prevBtn = new QPushButton("<", this);
    nextBtn = new QPushButton(">", this);
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(400, 300);
    imageLabel->setStyleSheet("background:#222;color:white;");

    // 布局
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(prevBtn);
    layout->addWidget(imageLabel, 1);
    layout->addWidget(nextBtn);
    setLayout(layout);

    // 连接信号槽
    connect(prevBtn, &QPushButton::clicked, this, &Picture::onPrevClicked);
    connect(nextBtn, &QPushButton::clicked, this, &Picture::onNextClicked);

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
        return;
    }
    QPixmap pix(imageFiles[currentIndex]);
    if (pix.isNull()) {
        imageLabel->setText("图片加载失败");
    } else {
        imageLabel->setPixmap(pix.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
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