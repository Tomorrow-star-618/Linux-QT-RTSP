#ifdef PLATFORM_RK3576

#include "MppDecoder.h"
#include <cstring>
#include <QDateTime>

MppDecoder::MppDecoder()
    : m_mppCtx(nullptr)
    , m_mppApi(nullptr)
    , m_frmGrp(nullptr)
    , m_width(0)
    , m_height(0)
    , m_initialized(false)
    , m_useRGA(true)  // 默认启用 RGA
    , m_rgbBuffer(nullptr)
    , m_rgbBufferSize(0)
    , m_rgbMppBuffer(nullptr)
{
    // 初始化 RGA
    int rgaRet = c_RkRgaInit();
    if (rgaRet != 0) {
        qWarning() << "MppDecoder: RGA 初始化失败，将使用 CPU 转换" << rgaRet;
        m_useRGA = false;
    }
}

MppDecoder::~MppDecoder()
{
    cleanup();
    
    // 释放 RGA 资源
    if (m_useRGA) {
        c_RkRgaDeInit();
    }
}

bool MppDecoder::init(AVCodecParameters* codecpar)
{
    MPP_RET ret = MPP_OK;
    
    // 创建 MPP 上下文
    ret = mpp_create(&m_mppCtx, &m_mppApi);
    if (ret != MPP_OK) {
        qWarning() << "MppDecoder: 创建 MPP 上下文失败" << ret;
        return false;
    }
    
    // 设置解码器类型
    MppCtxType type = MPP_CTX_DEC;
    ret = mpp_init(m_mppCtx, type, MPP_VIDEO_CodingAVC); // H.264
    if (ret != MPP_OK) {
        qWarning() << "MppDecoder: 初始化 MPP 失败" << ret;
        cleanup();
        return false;
    }
    
    // 设置解码器参数 - 关键优化
    MppDecCfg cfg = NULL;
    mpp_dec_cfg_init(&cfg);
    if (cfg) {
        // 设置快速输出模式，减少延迟
        mpp_dec_cfg_set_u32(cfg, "base:fast_out", 1);
        // 禁用错误隐藏，加快处理速度
        mpp_dec_cfg_set_u32(cfg, "base:disable_error", 0);
        m_mppApi->control(m_mppCtx, MPP_DEC_SET_CFG, cfg);
        mpp_dec_cfg_deinit(cfg);
    }
    
    // 获取视频尺寸
    m_width = codecpar->width;
    m_height = codecpar->height;
    
    // 创建帧缓冲组
    ret = mpp_buffer_group_get_internal(&m_frmGrp, MPP_BUFFER_TYPE_ION);
    if (ret != MPP_OK) {
        qWarning() << "MppDecoder: 创建帧缓冲组失败" << ret;
        cleanup();
        return false;
    }
    
    // 设置帧缓冲组
    ret = m_mppApi->control(m_mppCtx, MPP_DEC_SET_EXT_BUF_GROUP, m_frmGrp);
    if (ret != MPP_OK) {
        qWarning() << "MppDecoder: 设置帧缓冲组失败" << ret;
        cleanup();
        return false;
    }
    
    // 分配 RGB 缓冲区
    m_rgbBufferSize = m_width * m_height * 3;
    
    if (m_useRGA) {
        // 使用 MPP buffer 以便 RGA 可以直接访问
        MPP_RET ret = mpp_buffer_get(m_frmGrp, &m_rgbMppBuffer, m_rgbBufferSize);
        if (ret == MPP_OK) {
            m_rgbBuffer = (uint8_t*)mpp_buffer_get_ptr(m_rgbMppBuffer);
        } else {
            m_rgbBuffer = new uint8_t[m_rgbBufferSize];
            m_useRGA = false;
        }
    } else {
        m_rgbBuffer = new uint8_t[m_rgbBufferSize];
    }
    
    if (!m_rgbBuffer) {
        qWarning() << "MppDecoder: 分配 RGB 缓冲区失败";
        cleanup();
        return false;
    }
    
    m_initialized = true;
    return true;
}

bool MppDecoder::sendPacket(AVPacket* packet)
{
    if (!m_initialized || !m_mppApi) return false;
    
    MPP_RET ret = MPP_OK;
    MppPacket mppPacket = nullptr;
    
    // 创建 MPP 数据包
    ret = mpp_packet_init(&mppPacket, packet->data, packet->size);
    if (ret != MPP_OK) {
        qWarning() << "MppDecoder: 初始化 MPP 数据包失败";
        return false;
    }
    
    // 设置时间戳
    mpp_packet_set_pts(mppPacket, packet->pts);
    
    // 发送数据包到解码器
    ret = m_mppApi->decode_put_packet(m_mppCtx, mppPacket);
    
    // 释放数据包
    mpp_packet_deinit(&mppPacket);
    
    if (ret != MPP_OK) {
        // MPP_ERR_BUFFER_FULL 表示缓冲区满，需要先取出帧
        if (ret != MPP_ERR_BUFFER_FULL) {
            qWarning() << "MppDecoder: 发送数据包失败" << ret;
            return false;
        }
    }
    
    return true;
}

bool MppDecoder::receiveFrame(QImage& image)
{
    if (!m_initialized || !m_mppApi) return false;
    
    MPP_RET ret = MPP_OK;
    MppFrame mppFrame = nullptr;
    
    // 从解码器获取帧
    ret = m_mppApi->decode_get_frame(m_mppCtx, &mppFrame);
    if (ret != MPP_OK || mppFrame == nullptr) {
        return false;
    }
    
    // 检查帧是否有效
    if (mpp_frame_get_info_change(mppFrame)) {
        // 分辨率变化，更新参数
        m_width = mpp_frame_get_width(mppFrame);
        m_height = mpp_frame_get_height(mppFrame);
        
        // 重新分配 RGB 缓冲区
        m_rgbBufferSize = m_width * m_height * 3;
        
        // 正确释放旧缓冲区
        if (m_rgbMppBuffer) {
            mpp_buffer_put(m_rgbMppBuffer);
            m_rgbMppBuffer = nullptr;
            m_rgbBuffer = nullptr;
        } else if (m_rgbBuffer) {
            delete[] m_rgbBuffer;
            m_rgbBuffer = nullptr;
        }
        
        // 重新分配新缓冲区
        if (m_useRGA) {
            MPP_RET ret = mpp_buffer_get(m_frmGrp, &m_rgbMppBuffer, m_rgbBufferSize);
            if (ret == MPP_OK) {
                m_rgbBuffer = (uint8_t*)mpp_buffer_get_ptr(m_rgbMppBuffer);
            } else {
                m_rgbBuffer = new uint8_t[m_rgbBufferSize];
                m_useRGA = false;
            }
        } else {
            m_rgbBuffer = new uint8_t[m_rgbBufferSize];
        }
        
        mpp_frame_deinit(&mppFrame);
        
        // 需要告诉 MPP 已经处理了 info_change
        m_mppApi->control(m_mppCtx, MPP_DEC_SET_INFO_CHANGE_READY, nullptr);
        
        return false;
    }
    
    // 检查是否为 EOS (End of Stream)
    if (mpp_frame_get_eos(mppFrame)) {
        mpp_frame_deinit(&mppFrame);
        return false;
    }
    
    // 检查帧错误标志
    RK_U32 errInfo = mpp_frame_get_errinfo(mppFrame);
    
    // 只在严重错误时才丢弃帧
    if (errInfo && (errInfo & MPP_FRAME_ERR_UNKNOW)) {
        mpp_frame_deinit(&mppFrame);
        return false;
    }
    
    // 转换 NV12 到 RGB - 优先使用 RGA 硬件加速
    bool success = false;
    if (m_useRGA) {
        success = convertNV12ToRGBbyRGA(mppFrame, image);
        if (!success) {
            // RGA 失败时回退到 CPU
            m_useRGA = false;
            success = convertNV12ToRGBbyCPU(mppFrame, image);
        }
    } else {
        success = convertNV12ToRGBbyCPU(mppFrame, image);
    }
    
    // 释放帧
    mpp_frame_deinit(&mppFrame);
    
    return success;
}

bool MppDecoder::convertNV12ToRGBbyRGA(MppFrame frame, QImage& image)
{
    // 获取 MPP 帧缓冲区
    MppBuffer srcBuffer = mpp_frame_get_buffer(frame);
    if (!srcBuffer) {
        return false;
    }
    
    int srcFd = mpp_buffer_get_fd(srcBuffer);
    int yStride = mpp_frame_get_hor_stride(frame);
    int yHeight = mpp_frame_get_ver_stride(frame);
    
    // 验证参数合理性
    if (srcFd < 0 || yStride < m_width || yHeight < m_height) {
        return false;
    }
    
    // 配置 RGA 源信息（NV12 格式）
    rga_info_t src;
    memset(&src, 0, sizeof(rga_info_t));
    src.fd = srcFd;
    src.mmuFlag = 1;
    rga_set_rect(&src.rect, 0, 0, m_width, m_height, yStride, yHeight, RK_FORMAT_YCbCr_420_SP);
    
    // 配置 RGA 目标信息（RGB888 格式）
    rga_info_t dst;
    memset(&dst, 0, sizeof(rga_info_t));
    
    // RGA wstride 是像素宽度，需要 16 字节对齐
    int dstWStride = (m_width + 15) & ~15;
    
    if (m_rgbMppBuffer) {
        int dstFd = mpp_buffer_get_fd(m_rgbMppBuffer);
        if (dstFd < 0) {
            return false;
        }
        dst.fd = dstFd;
        dst.mmuFlag = 1;
    } else {
        dst.virAddr = m_rgbBuffer;
        dst.mmuFlag = 1;
    }
    
    rga_set_rect(&dst.rect, 0, 0, m_width, m_height, dstWStride, m_height, RK_FORMAT_RGB_888);
    
    // 执行 RGA 转换
    int ret = c_RkRgaBlit(&src, &dst, nullptr);
    if (ret != 0) {
        return false;
    }
    
    // 创建 QImage - RGB888 每像素3字节
    int bytesPerLine = dstWStride * 3;
    image = QImage(m_rgbBuffer, m_width, m_height, bytesPerLine, QImage::Format_RGB888).copy();
    
    return true;
}

bool MppDecoder::convertNV12ToRGBbyCPU(MppFrame frame, QImage& image)
{
    // CPU 软件转换（备用方案）
    MppBuffer buffer = mpp_frame_get_buffer(frame);
    if (!buffer) {
        return false;
    }
    
    uint8_t* yPlane = (uint8_t*)mpp_buffer_get_ptr(buffer);
    if (!yPlane) {
        return false;
    }
    
    int yStride = mpp_frame_get_hor_stride(frame);
    int uvStride = yStride;
    uint8_t* uvPlane = yPlane + yStride * m_height;
    
    // 整数运算 YUV 转 RGB
    for (int y = 0; y < m_height; y++) {
        uint8_t* yRow = yPlane + y * yStride;
        uint8_t* uvRow = uvPlane + (y / 2) * uvStride;
        uint8_t* rgbRow = m_rgbBuffer + y * m_width * 3;
        
        for (int x = 0; x < m_width; x++) {
            int Y = yRow[x];
            int uvIndex = (x & ~1);
            int U = uvRow[uvIndex] - 128;
            int V = uvRow[uvIndex + 1] - 128;
            
            int R = Y + ((1436 * V) >> 10);
            int G = Y - ((352 * U + 731 * V) >> 10);
            int B = Y + ((1815 * U) >> 10);
            
            R = (R < 0) ? 0 : ((R > 255) ? 255 : R);
            G = (G < 0) ? 0 : ((G > 255) ? 255 : G);
            B = (B < 0) ? 0 : ((B > 255) ? 255 : B);
            
            int rgbIndex = x * 3;
            rgbRow[rgbIndex] = R;
            rgbRow[rgbIndex + 1] = G;
            rgbRow[rgbIndex + 2] = B;
        }
    }
    
    image = QImage(m_rgbBuffer, m_width, m_height, m_width * 3, QImage::Format_RGB888).copy();
    return true;
}

void MppDecoder::cleanup()
{
    if (m_rgbMppBuffer) {
        mpp_buffer_put(m_rgbMppBuffer);
        m_rgbMppBuffer = nullptr;
        m_rgbBuffer = nullptr;
    } else if (m_rgbBuffer) {
        delete[] m_rgbBuffer;
        m_rgbBuffer = nullptr;
    }
    
    if (m_frmGrp) {
        mpp_buffer_group_put(m_frmGrp);
        m_frmGrp = nullptr;
    }
    
    if (m_mppCtx) {
        mpp_destroy(m_mppCtx);
        m_mppCtx = nullptr;
        m_mppApi = nullptr;
    }
    
    m_initialized = false;
}

#endif // PLATFORM_RK3576
