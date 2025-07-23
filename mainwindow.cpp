// mainwindow.cpp
#include "mainwindow.h"
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QDebug>

// 引入FFmpeg相关头文件
extern "C" {
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>   // 图像工具函数声明
}

// RtspThread线程实现，负责拉流、解码、转换为QImage并发射信号
RtspThread::RtspThread(QObject* parent) : QThread(parent), m_stop(false) {}
RtspThread::~RtspThread() { stopStream(); wait(); }

// 启动拉流
void RtspThread::startStream(const QString& url) {
    QMutexLocker locker(&m_mutex);
    m_url = url;
    m_stop = false;
    if (!isRunning()) start(); // 如果线程未启动则启动
    else m_wait.wakeOne();     // 已启动则唤醒线程
}
// 停止拉流
void RtspThread::stopStream() {
    QMutexLocker locker(&m_mutex);
    m_stop = true;
    m_wait.wakeOne();
}

// 线程主函数，循环拉流并解码
void RtspThread::run() {
    avformat_network_init(); // 初始化FFmpeg网络模块
    while (true) {
        m_mutex.lock();
        QString url = m_url;
        bool stop = m_stop;
        m_mutex.unlock();
        if (stop) break;

        // 打开RTSP流
        AVFormatContext* fmt_ctx = nullptr;
        if (avformat_open_input(&fmt_ctx, url.toStdString().c_str(), nullptr, nullptr) != 0) {
            emit frameReady(QImage()); // 打开失败，发空帧
            QThread::msleep(1000);
            continue;
        }
        // 读取流信息
        if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
            avformat_close_input(&fmt_ctx);
            emit frameReady(QImage());
            QThread::msleep(1000);
            continue;
        }
        // 查找视频流索引
        int videoStream = -1;
        for (unsigned i = 0; i < fmt_ctx->nb_streams; ++i) {
            if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStream = i; break;
            }
        }
        if (videoStream == -1) {
            avformat_close_input(&fmt_ctx);
            emit frameReady(QImage());
            QThread::msleep(1000);
            continue;
        }
        // 查找解码器
        AVCodecParameters* codecpar = fmt_ctx->streams[videoStream]->codecpar;
        AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
        AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codec_ctx, codecpar);
        avcodec_open2(codec_ctx, codec, nullptr);

        // 分配帧和转换上下文
        AVFrame* frame = av_frame_alloc();
        AVFrame* rgbFrame = av_frame_alloc();
        SwsContext* sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
                                             codec_ctx->width, codec_ctx->height, AV_PIX_FMT_RGB24,
                                             SWS_BILINEAR, nullptr, nullptr, nullptr);
        // 分配RGB缓冲区
        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height, 1);
        uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
        av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24,
                             codec_ctx->width, codec_ctx->height, 1);

        AVPacket pkt;
        // 主循环：读取、解码、转换、发信号
        while (!m_stop && av_read_frame(fmt_ctx, &pkt) >= 0) {
            if (pkt.stream_index == videoStream) {
                if (avcodec_send_packet(codec_ctx, &pkt) == 0) {
                    while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                        // YUV转RGB
                        sws_scale(sws_ctx, frame->data, frame->linesize, 0, codec_ctx->height,
                                  rgbFrame->data, rgbFrame->linesize);
                        // 封装为QImage
                        QImage img(rgbFrame->data[0], codec_ctx->width, codec_ctx->height,
                                   rgbFrame->linesize[0], QImage::Format_RGB888);
                        emit frameReady(img.copy()); // 发送信号到主线程
                    }
                }
            }
            av_packet_unref(&pkt);
            m_mutex.lock();
            if (m_stop) { m_mutex.unlock(); break; }
            m_mutex.unlock();
        }
        // 释放资源
        av_free(buffer);
        av_frame_free(&rgbFrame);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);

        m_mutex.lock();
        if (!m_stop) m_wait.wait(&m_mutex); // 等待新流或停止
        m_mutex.unlock();
    }
}

// 主窗口实现
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), rtspThread(new RtspThread(this)) {
    resize(800, 480);
    videoLabel = new QLabel(this);
    videoLabel->setAlignment(Qt::AlignCenter);
    videoLabel->setMinimumSize(600, 480);
    videoLabel->setGeometry(0, 0, 600, 480);

    addCameraBtn = new QPushButton("添加摄像头URL地址", this);
    addCameraBtn->setGeometry(650, 200, 130, 40);
    connect(addCameraBtn, &QPushButton::clicked, this, &MainWindow::onAddCameraClicked);
    connect(rtspThread, &RtspThread::frameReady, this, &MainWindow::onFrameReady);
}
MainWindow::~MainWindow() {
    rtspThread->stopStream();
    rtspThread->wait();
}

// 点击按钮弹出输入框，输入RTSP地址并启动拉流
void MainWindow::onAddCameraClicked() {
    QInputDialog inputDialog(this);
    inputDialog.setWindowTitle("输入RTSP地址");
    inputDialog.setLabelText("RTSP URL:");
    inputDialog.setTextValue("rtsp://192.168.1.150/live/0");
    QLineEdit *lineEdit = inputDialog.findChild<QLineEdit *>();
    if (lineEdit) lineEdit->setMinimumWidth(500);
    int ret = inputDialog.exec();
    QString url = inputDialog.textValue();
    if (ret != QDialog::Accepted || url.isEmpty()) {
        QMessageBox::warning(this, "错误", "未输入RTSP地址");
        return;
    }
    rtspThread->startStream(url);
}

// 接收解码线程的信号，显示视频帧
void MainWindow::onFrameReady(const QImage& img)
{
    if (!img.isNull()) {
        videoLabel->setPixmap(QPixmap::fromImage(img).scaled(videoLabel->size(), Qt::KeepAspectRatio));
    }
}
