#pragma once
#include <QObject>
#include <QMap>
#include "model.h"
#include "view.h"
#include "Picture.h"
#include "Tcpserver.h"
#include "VideoLabel.h"  // 包含RectangleBox定义
#include "detectlist.h"  // 包含DetectList类

class Plan; // 前向声明

// 方案数据结构前向声明
struct PlanData;

class Controller : public QObject {
    Q_OBJECT
public:
    Controller(Model* model, View* view, QObject* parent = nullptr);
    ~Controller();
    
    // 设置TCP服务器指针
    void setTcpServer(Tcpserver* tcpServer);
    
    // 多路视频流管理
    void addVideoStream(const QString& url, const QString& name, int cameraId);
    void removeVideoStream(int streamId);
    void clearAllStreams();

public slots:
    void ButtonClickedHandler();      //主界面标签按键槽
    void ServoButtonClickedHandler(); //云台按键槽
    void FunButtonClickedHandler();   //功能按钮槽
    void onTcpClientConnected(const QString& ip, quint16 port); // 新增：处理TCP客户端连接成功

        
private slots:
    void onAddCameraClicked();      //添加摄像头槽
    void onFrameReady(const QImage& img); //视频帧槽
    void onDetectListSelectionChanged(const QSet<int>& selectedIds); //对象列表选择变化槽 
    void onRectangleConfirmed(const RectangleBox& rect);// 处理用户确认的矩形框（绝对坐标），用于目标选定等功能
    // 处理用户确认的矩形框（归一化坐标和绝对坐标），便于后续处理如检测、标注等
    void onNormalizedRectangleConfirmed(const NormalizedRectangleBox& normRect, const RectangleBox& absRect);
    void onPlanApplied(const PlanData& plan); // 处理方案应用槽
    void onDetectionDataReceived(const QString& detectionData); // 新增：处理检测数据接收槽
    
    // 多路视频流槽函数
    void onLayoutModeChanged(int mode);     // 布局模式切换
    void onStreamSelected(int streamId);    // 视频流选择
    void onModelFrameReady(int streamId, const QImage& frame); // 多路视频帧更新
    void onStreamPauseRequested(int streamId);     // 暂停视频流
    void onStreamScreenshotRequested(int streamId); // 截图视频流
    void onAddCameraWithIdRequested(int cameraId); // 添加指定ID的摄像头

private:
    Model* m_model; //模型指针  
    View* m_view;   //视图指针
    bool m_paused = false; //暂停标志
    QImage m_lastImage; // 保存最近一帧图像
    void saveImage();   // 截图保存函数
    void saveAlarmImage(const QString& detectionInfo); // 新增：报警图像保存函数
    Tcpserver* tcpWin = nullptr; // TCP服务器窗口指针
    DetectList* m_detectList = nullptr; // 对象检测列表窗口指针
    Plan* m_plan = nullptr; // 方案预选窗口指针
    QSet<int> m_selectedObjectIds; // 当前选中的对象ID集合
    
    // 功能按钮状态管理
    void updateButtonDependencies(int clickedButtonId, bool isChecked);
    
    // 多路视频流管理
    QMap<int, Model*> m_streamModels;  // streamId -> Model映射
    int m_nextStreamId;                // 下一个可用的流ID
};
