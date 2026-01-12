#include "FFmpegDecoder.h"
#include <QDebug>

FFmpegDecoder::FFmpegDecoder()
    : m_codecCtx(nullptr)
    , m_frame(nullptr)
    , m_rgbFrame(nullptr)
    , m_swsCtx(nullptr)
    , m_buffer(nullptr)
    , m_width(0)
    , m_height(0)
{
}

FFmpegDecoder::~FFmpegDecoder()
{
    cleanup();
}

bool FFmpegDecoder::init(AVCodecParameters* codecpar)
{
    // 查找解码器
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        qWarning() << "FFmpegDecoder: 未找到解码器";
        return false;
    }
    
    // 分配解码器上下文
    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) {
        qWarning() << "FFmpegDecoder: 无法分配解码器上下文";
        return false;
    }
    
    // 拷贝参数
    if (avcodec_parameters_to_context(m_codecCtx, codecpar) < 0) {
        qWarning() << "FFmpegDecoder: 无法拷贝解码器参数";
        cleanup();
        return false;
    }
    
    // 打开解码器
    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        qWarning() << "FFmpegDecoder: 无法打开解码器";
        cleanup();
        return false;
    }
    
    m_width = m_codecCtx->width;
    m_height = m_codecCtx->height;
    
    // 分配帧
    m_frame = av_frame_alloc();
    m_rgbFrame = av_frame_alloc();
    if (!m_frame || !m_rgbFrame) {
        qWarning() << "FFmpegDecoder: 无法分配帧";
        cleanup();
        return false;
    }
    
    // 创建图像转换上下文
    m_swsCtx = sws_getContext(
        m_width, m_height, m_codecCtx->pix_fmt,
        m_width, m_height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    
    if (!m_swsCtx) {
        qWarning() << "FFmpegDecoder: 无法创建图像转换上下文";
        cleanup();
        return false;
    }
    
    // 分配 RGB 缓冲区
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_width, m_height, 1);
    m_buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
    if (!m_buffer) {
        qWarning() << "FFmpegDecoder: 无法分配缓冲区";
        cleanup();
        return false;
    }
    
    av_image_fill_arrays(m_rgbFrame->data, m_rgbFrame->linesize, m_buffer,
                        AV_PIX_FMT_RGB24, m_width, m_height, 1);
    
    qDebug() << "FFmpegDecoder: 软件解码器初始化成功" << m_width << "x" << m_height;
    return true;
}

bool FFmpegDecoder::sendPacket(AVPacket* packet)
{
    if (!m_codecCtx) return false;
    return avcodec_send_packet(m_codecCtx, packet) == 0;
}

bool FFmpegDecoder::receiveFrame(QImage& image)
{
    if (!m_codecCtx) return false;
    
    if (avcodec_receive_frame(m_codecCtx, m_frame) != 0) {
        return false;
    }
    
    // 转换为 RGB 格式
    sws_scale(m_swsCtx, m_frame->data, m_frame->linesize, 0, m_height,
              m_rgbFrame->data, m_rgbFrame->linesize);
    
    // 构造 QImage
    image = QImage(m_rgbFrame->data[0], m_width, m_height,
                   m_rgbFrame->linesize[0], QImage::Format_RGB888).copy();
    
    return true;
}

void FFmpegDecoder::cleanup()
{
    if (m_buffer) {
        av_free(m_buffer);
        m_buffer = nullptr;
    }
    if (m_rgbFrame) {
        av_frame_free(&m_rgbFrame);
    }
    if (m_frame) {
        av_frame_free(&m_frame);
    }
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
    }
}
