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
    } else {
        qDebug() << "MppDecoder: ✓ RGA 硬件加速已启用";
    }
}

MppDecoder::~MppDecoder()
{
    cleanup();
    
    // 释放 RGA 资源
    if (m_useRGA) {
        c_RkRgaDeInit();
        qDebug() << "MppDecoder: RGA 资源已释放";
    }
}

bool MppDecoder::init(AVCodecParameters* codecpar)
{
    MPP_RET ret = MPP_OK;
    
    qDebug() << "MppDecoder: 初始化硬件解码器...";
    
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
            qDebug() << "MppDecoder: 分配 RGA 输出缓冲区成功";
        } else {
            qWarning() << "MppDecoder: 分配 MPP buffer 失败，回退到普通缓冲区";
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
    qDebug() << "MppDecoder: ✓ 硬件解码器初始化成功" << m_width << "x" << m_height 
             << (m_useRGA ? "(使用 RGA 加速)" : "(使用 CPU 转换)");
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
        
        qDebug() << "MppDecoder: 分辨率变化" << m_width << "x" << m_height;
        
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
    
    // 检查帧错误标志 - 修改处理策略
    RK_U32 errInfo = mpp_frame_get_errinfo(mppFrame);
    RK_U32 discard = mpp_frame_get_discard(mppFrame);
    
    // 只在严重错误时才丢弃帧
    if (errInfo && (errInfo & MPP_FRAME_ERR_UNKNOW)) {
        static int errorCount = 0;
        if (++errorCount % 10 == 0) {
            qWarning() << "MppDecoder: 严重帧错误，已丢弃" << errorCount << "帧";
        }
        mpp_frame_deinit(&mppFrame);
        return false;
    }
    
    // 轻微错误的帧仍然尝试转换显示
    if (discard || errInfo) {
        static int minorErrorCount = 0;
        if (++minorErrorCount % 50 == 0) {
            qDebug() << "MppDecoder: 轻微错误帧，继续处理 (已处理" << minorErrorCount << "个)";
        }
    }
    
    // 转换 NV12 到 RGB - 优先使用 RGA 硬件加速
    bool success = false;
    if (m_useRGA) {
        success = convertNV12ToRGBbyRGA(mppFrame, image);
        if (!success) {
            // RGA 失败时回退到 CPU
            qWarning() << "MppDecoder: RGA 转换失败，回退到 CPU";
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
    static int callCount = 0;
    static bool debugMode = true;  // 只在前几帧输出调试信息
    callCount++;
    
    // 前5帧或失败时才输出详细调试信息
    bool shouldDebug = (callCount <= 5) || !debugMode;
    
    // 获取 MPP 帧缓冲区
    MppBuffer srcBuffer = mpp_frame_get_buffer(frame);
    if (!srcBuffer) {
        qWarning() << "MppDecoder: [RGA调试] 步骤1失败 - 无法获取源缓冲区";
        return false;
    }
    if (shouldDebug) qDebug() << "MppDecoder: [RGA调试 #" << callCount << "] 步骤1✓ - 获取源缓冲区成功";
    
    int srcFd = mpp_buffer_get_fd(srcBuffer);
    int yStride = mpp_frame_get_hor_stride(frame);
    int yHeight = mpp_frame_get_ver_stride(frame);
    
    if (shouldDebug) qDebug() << "MppDecoder: [RGA调试] 步骤2 - 源FD:" << srcFd << "stride:" << yStride << "height:" << yHeight;
    
    // 验证参数合理性
    if (srcFd < 0) {
        qWarning() << "MppDecoder: [RGA调试] 步骤2失败 - RGA 源 FD 无效:" << srcFd;
        return false;
    }
    
    if (yStride < m_width || yHeight < m_height) {
        qWarning() << "MppDecoder: [RGA调试] 步骤2失败 - stride参数异常 - stride:" << yStride << "height:" << yHeight 
                   << "实际分辨率:" << m_width << "x" << m_height;
        return false;
    }
    if (shouldDebug) qDebug() << "MppDecoder: [RGA调试] 步骤2✓ - 参数验证通过";
    
    // 配置 RGA 源信息（NV12 格式）
    rga_info_t src;
    memset(&src, 0, sizeof(rga_info_t));
    src.fd = srcFd;
    src.mmuFlag = 1;  // 使用 MMU
    
    // 使用正确的 stride 和 height
    int srcRet = rga_set_rect(&src.rect, 0, 0, m_width, m_height, yStride, yHeight, RK_FORMAT_YCbCr_420_SP);
    if (shouldDebug) qDebug() << "MppDecoder: [RGA调试] 步骤3 - 源rect配置返回:" << srcRet;
    
    // 配置 RGA 目标信息（RGB888 格式）
    rga_info_t dst;
    memset(&dst, 0, sizeof(rga_info_t));
    
    // RGA wstride 是像素宽度，不是字节宽度
    // 对于 RGB888，像素宽度需要 16 字节对齐 (即像素数对齐到 16)
    int dstWStride = (m_width + 15) & ~15;  // 像素宽度对齐到 16
    
    if (shouldDebug) qDebug() << "MppDecoder: [RGA调试] 步骤4 - 目标wstride(像素):" << dstWStride << "(原始宽度:" << m_width << ")";
    
    if (m_rgbMppBuffer) {
        // 使用 MPP buffer（推荐，零拷贝）
        int dstFd = mpp_buffer_get_fd(m_rgbMppBuffer);
        if (shouldDebug) qDebug() << "MppDecoder: [RGA调试] 步骤4 - 使用MPP buffer, FD:" << dstFd;
        
        if (dstFd < 0) {
            qWarning() << "MppDecoder: [RGA调试] 步骤4失败 - RGA 目标 FD 无效:" << dstFd;
            return false;
        }
        dst.fd = dstFd;
        dst.mmuFlag = 1;
    } else {
        // 使用虚拟地址
        if (shouldDebug) qDebug() << "MppDecoder: [RGA调试] 步骤4 - 使用虚拟地址:" << (void*)m_rgbBuffer;
        dst.virAddr = m_rgbBuffer;
        dst.mmuFlag = 1;
    }
    
    int dstRet = rga_set_rect(&dst.rect, 0, 0, m_width, m_height, dstWStride, m_height, RK_FORMAT_RGB_888);
    if (shouldDebug) qDebug() << "MppDecoder: [RGA调试] 步骤4✓ - 目标rect配置返回:" << dstRet;
    
    // 性能统计
    static int frameCount = 0;
    static qint64 lastTime = 0;
    static bool firstCall = true;
    frameCount++;
    
    // 首次调用时输出详细参数用于调试
    if (firstCall) {
        int dstFd = m_rgbMppBuffer ? mpp_buffer_get_fd(m_rgbMppBuffer) : -1;
        qDebug() << "========================================";
        qDebug() << "MppDecoder: [RGA 首次调用参数]";
        qDebug() << "  源 FD:" << srcFd << "目标 FD:" << dstFd;
        qDebug() << "  分辨率:" << m_width << "x" << m_height;
        qDebug() << "  源 wstride(像素):" << yStride << "hstride:" << yHeight;
        qDebug() << "  目标 wstride(像素):" << dstWStride << "hstride:" << m_height;
        qDebug() << "  使用 MPP buffer:" << (m_rgbMppBuffer != nullptr);
        qDebug() << "  源格式: RK_FORMAT_YCbCr_420_SP (0x" << QString::number(RK_FORMAT_YCbCr_420_SP, 16) << ")";
        qDebug() << "  目标格式: RK_FORMAT_RGB_888 (0x" << QString::number(RK_FORMAT_RGB_888, 16) << ")";
        qDebug() << "========================================";
        firstCall = false;
    }
    
    if (shouldDebug) qDebug() << "MppDecoder: [RGA调试] 步骤5 - 准备调用 c_RkRgaBlit...";
    
    // 执行 RGA 转换（硬件加速）
    int ret = c_RkRgaBlit(&src, &dst, nullptr);
    
    if (shouldDebug) qDebug() << "MppDecoder: [RGA调试] 步骤5 - c_RkRgaBlit 返回值:" << ret;
    
    if (ret != 0) {
        // 只在前5次失败时输出详细错误
        if (callCount <= 5) {
            qWarning() << "========================================";
            qWarning() << "MppDecoder: [RGA调试] 步骤5失败 ❌ RGA 转换失败";
            qWarning() << "  调用次数:" << callCount;
            qWarning() << "  错误码:" << ret;
            qWarning() << "  错误详情:";
            qWarning() << "    -22 = EINVAL (无效参数)";
            qWarning() << "    -1 = 一般错误";
            qWarning() << "    0 = 成功";
            qWarning() << "========================================";
        }
        debugMode = false;  // 失败后关闭详细调试
        return false;
    }
    
    // 第5帧成功时输出成功消息
    if (callCount == 5) {
        qDebug() << "========================================";
        qDebug() << "MppDecoder: ✅ RGA硬件转换连续成功! 关闭详细调试输出";
        qDebug() << "========================================";
    }
    
    // 每30帧统计一次性能
    if (frameCount % 30 == 0) {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        if (lastTime > 0) {
            double fps = 30000.0 / (currentTime - lastTime);
            qDebug() << "MppDecoder: [RGA 硬件] 转换帧率" << QString::number(fps, 'f', 2) << "fps";
        }
        lastTime = currentTime;
    }
    
    // 创建 QImage - RGB888 每像素3字节，stride = wstride * 3
    int bytesPerLine = dstWStride * 3;
    image = QImage(m_rgbBuffer, m_width, m_height, bytesPerLine, QImage::Format_RGB888).copy();
    
    return true;
}

bool MppDecoder::convertNV12ToRGBbyCPU(MppFrame frame, QImage& image)
{
    // CPU 软件转换（备用方案）
    MppBuffer buffer = mpp_frame_get_buffer(frame);
    if (!buffer) {
        qWarning() << "MppDecoder: 无法获取帧缓冲区";
        return false;
    }
    
    uint8_t* yPlane = (uint8_t*)mpp_buffer_get_ptr(buffer);
    if (!yPlane) {
        qWarning() << "MppDecoder: 无法获取缓冲区指针";
        return false;
    }
    
    int yStride = mpp_frame_get_hor_stride(frame);
    int uvStride = yStride;
    uint8_t* uvPlane = yPlane + yStride * m_height;
    
    static int frameCount = 0;
    frameCount++;
    
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
    
    // 性能统计
    if (frameCount % 30 == 0) {
        static qint64 lastTime = 0;
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        if (lastTime > 0) {
            double fps = 30000.0 / (currentTime - lastTime);
            qDebug() << "MppDecoder: [CPU 软件] 转换帧率" << QString::number(fps, 'f', 2) << "fps";
        }
        lastTime = currentTime;
    }
    
    image = QImage(m_rgbBuffer, m_width, m_height, m_width * 3, QImage::Format_RGB888).copy();
    return true;
}

void MppDecoder::cleanup()
{
    qDebug() << "MppDecoder: 清理资源...";
    
    if (m_rgbMppBuffer) {
        mpp_buffer_put(m_rgbMppBuffer);
        m_rgbMppBuffer = nullptr;
        m_rgbBuffer = nullptr;  // 指针已失效
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
