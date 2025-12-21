#include "model.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

Model::Model(QObject* parent)
    : QThread(parent), m_stop(false)
{
}

Model::~Model()
{
    stopStream();
    wait();
}

// 启动视频流线程
void Model::startStream(const QString& url)
{
    QMutexLocker locker(&m_mutex); // 加锁，保证线程安全
    m_url = url;                   // 设置RTSP流地址
    m_stop = false;                // 标记为未停止
    if (!isRunning())              // 如果线程未运行，则启动线程
        start();
    else                           // 如果线程已在运行，则唤醒等待的线程
        m_wait.wakeOne();
}

// 停止视频流线程
void Model::stopStream()
{
    QMutexLocker locker(&m_mutex); // 加锁，保证线程安全
    m_stop = true;                 // 标记为停止
    m_wait.wakeOne();              // 唤醒线程以便及时退出
}

// 暂停视频流
void Model::pauseStream()
{
    QMutexLocker locker(&m_mutex); // 加锁，保证线程安全
    m_pause = true;                // 设置暂停标志
}

// 恢复视频流
void Model::resumeStream()
{
    QMutexLocker locker(&m_mutex); // 加锁，保证线程安全
    m_pause = false;               // 取消暂停标志
    m_wait.wakeOne();              // 唤醒线程继续处理
}

// 打开RTSP流，获取AVFormatContext
bool Model::openStream(const QString& url, AVFormatContext*& fmt_ctx) {
    // 尝试打开输入流
    if (avformat_open_input(&fmt_ctx, url.toStdString().c_str(), nullptr, nullptr) != 0) {
        emit frameReady(QImage()); // 打开失败，发送空帧
        QThread::msleep(1000);     // 等待1秒后重试
        return false;
    }
    // 查找流信息
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        avformat_close_input(&fmt_ctx);
        emit frameReady(QImage());
        QThread::msleep(1000);
        return false;
    }
    return true;
}

// 查找视频流索引
int Model::findVideoStream(AVFormatContext* fmt_ctx) {
    for (unsigned i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            return i; // 找到视频流
        }
    }
    return -1; // 未找到视频流
}

// 打开解码器，获取AVCodecContext
bool Model::openDecoder(AVFormatContext* fmt_ctx, int videoStream, AVCodecContext*& codec_ctx) {
    AVCodecParameters* codecpar = fmt_ctx->streams[videoStream]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id); // 查找解码器
    codec_ctx = avcodec_alloc_context3(codec);                 // 分配解码器上下文
    avcodec_parameters_to_context(codec_ctx, codecpar);        // 拷贝参数
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {        // 打开解码器
        avcodec_free_context(&codec_ctx);
        return false;
    }
    return true;
}

// 读取并解码视频帧，转换为QImage并发送信号
void Model::readAndDecodeFrames(AVFormatContext* fmt_ctx, AVCodecContext* codec_ctx, int videoStream) {
    AVFrame* frame = av_frame_alloc();      // 原始帧
    AVFrame* rgbFrame = av_frame_alloc();   // RGB帧
    
    // 检查帧分配是否成功
    if (!frame || !rgbFrame) {
        if (frame) av_frame_free(&frame);
        if (rgbFrame) av_frame_free(&rgbFrame);
        return;
    }
    
    // 创建图像转换上下文
    SwsContext* sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
                                         codec_ctx->width, codec_ctx->height, AV_PIX_FMT_RGB24,
                                         SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!sws_ctx) {
        av_frame_free(&frame);
        av_frame_free(&rgbFrame);
        return;
    }
    
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height, 1);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t)); // 分配RGB缓冲区
    if (!buffer) {
        av_frame_free(&frame);
        av_frame_free(&rgbFrame);
        sws_freeContext(sws_ctx);
        return;
    }
    
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24,
                        codec_ctx->width, codec_ctx->height, 1);
    AVPacket pkt;
    int readResult = 0;
    
    // 读取视频帧主循环
    while (!m_stop) {
        readResult = av_read_frame(fmt_ctx, &pkt);
        
        // 如果读取失败（推流端断开或其他错误）
        if (readResult < 0) {
            // 检查是否是EOF或连接断开
            if (readResult == AVERROR_EOF || readResult == AVERROR(EIO) || 
                readResult == AVERROR(EPIPE) || readResult == AVERROR(ECONNRESET)) {
                // 流结束或连接断开，正常退出循环，准备重连
                break;
            }
            // 其他错误，稍作延迟后继续尝试
            QThread::msleep(100);
            continue;
        }
        
        m_mutex.lock();
        if (m_pause) {
            m_wait.wait(&m_mutex); // 如果暂停，等待唤醒
        }
        m_mutex.unlock();
        
        if (pkt.stream_index == videoStream) {
            // 发送包到解码器
            if (avcodec_send_packet(codec_ctx, &pkt) == 0) {
                // 接收解码帧
                while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    // 转换为RGB格式
                    sws_scale(sws_ctx, frame->data, frame->linesize, 0, codec_ctx->height,
                              rgbFrame->data, rgbFrame->linesize);
                    // 构造QImage并发送信号
                    QImage img(rgbFrame->data[0], codec_ctx->width, codec_ctx->height,
                               rgbFrame->linesize[0], QImage::Format_RGB888);
                    emit frameReady(img.copy());
                }
            }
        }
        av_packet_unref(&pkt); // 释放包
        
        m_mutex.lock();
        if (m_stop) {
            m_mutex.unlock();
            break; // 如果需要停止，跳出循环
        }
        m_mutex.unlock();
    }
    
    // 释放帧和转换上下文等资源（注意：不在这里释放fmt_ctx和codec_ctx）
    if (buffer) av_free(buffer);
    if (rgbFrame) av_frame_free(&rgbFrame);
    if (frame) av_frame_free(&frame);
    if (sws_ctx) sws_freeContext(sws_ctx);
}

// 释放所有相关资源
void Model::cleanup(AVFormatContext* fmt_ctx, AVCodecContext* codec_ctx, AVFrame* frame, AVFrame* rgbFrame, uint8_t* buffer, SwsContext* sws_ctx) {
    if (buffer) av_free(buffer);           // 释放RGB缓冲区
    if (rgbFrame) av_frame_free(&rgbFrame);// 释放RGB帧
    if (frame) av_frame_free(&frame);      // 释放原始帧
    if (codec_ctx) avcodec_free_context(&codec_ctx); // 释放解码器上下文
    if (fmt_ctx) avformat_close_input(&fmt_ctx);     // 关闭输入流
    if (sws_ctx) sws_freeContext(sws_ctx);           // 释放图像转换上下文
}

// 主线程函数，负责整体流程调度
void Model::run()
{
    avformat_network_init(); // 初始化网络模块
    while (true)
    {
        // 获取当前url和停止标志
        m_mutex.lock();
        QString url = m_url;
        bool stop = m_stop;
        m_mutex.unlock();
        if (stop)
            break; // 需要停止时退出主循环
        AVFormatContext* fmt_ctx = nullptr;
        // 打开流
        if (!openStream(url, fmt_ctx))
            continue;
        // 查找视频流索引
        int videoStream = findVideoStream(fmt_ctx);
        if (videoStream == -1) {
            cleanup(fmt_ctx, nullptr, nullptr, nullptr, nullptr, nullptr);
            emit frameReady(QImage());
            QThread::msleep(1000);
            continue;
        }
        // 打开解码器
        AVCodecContext* codec_ctx = nullptr;
        if (!openDecoder(fmt_ctx, videoStream, codec_ctx)) {
            cleanup(fmt_ctx, codec_ctx, nullptr, nullptr, nullptr, nullptr);
            emit frameReady(QImage());
            QThread::msleep(1000);
            continue;
        }
        // 读取并解码帧
        readAndDecodeFrames(fmt_ctx, codec_ctx, videoStream);
        
        // 释放资源（fmt_ctx和codec_ctx在这里释放）
        if (codec_ctx) avcodec_free_context(&codec_ctx);
        if (fmt_ctx) avformat_close_input(&fmt_ctx);
        
        // 检查是否需要停止
        m_mutex.lock();
        bool shouldStop = m_stop;
        QString currentUrl = m_url;
        m_mutex.unlock();
        
        if (shouldStop) {
            break; // 如果需要停止，退出主循环
        }
        
        // 如果不是主动停止，说明是连接断开
        emit streamDisconnected(currentUrl);
        
        // 等待一段时间后尝试重连
        QThread::msleep(2000); // 等待2秒后重连
        
        // 发出重连信号
        emit streamReconnecting(currentUrl);
    }
} 