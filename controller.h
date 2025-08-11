#pragma once
#include <QObject>
#include "model.h"
#include "view.h"
#include "Picture.h"
#include "Tcpserver.h"
#include "VideoLabel.h"  // 包含RectangleBox定义
#include "detectlist.h"  // 包含DetectList类

class Controller : public QObject {
    Q_OBJECT
public:
    Controller(Model* model, View* view, QObject* parent = nullptr);
    ~Controller();
    
    // 设置TCP服务器指针
    void setTcpServer(Tcpserver* tcpServer);

public slots:
    void ButtonClickedHandler();      //主界面标签按键槽
    void ServoButtonClickedHandler(); //云台按键槽
    void FunButtonClickedHandler();   //功能按钮槽

        
private slots:
    void onAddCameraClicked();      //添加摄像头槽
    void onFrameReady(const QImage& img); //视频帧槽
    void onDetectListSelectionChanged(const QSet<int>& selectedIds); //对象列表选择变化槽 
    void onRectangleConfirmed(const RectangleBox& rect);// 处理用户确认的矩形框（绝对坐标），用于目标选定等功能
    // 处理用户确认的矩形框（归一化坐标和绝对坐标），便于后续处理如检测、标注等
    void onNormalizedRectangleConfirmed(const NormalizedRectangleBox& normRect, const RectangleBox& absRect);

private:
    Model* m_model; //模型指针  
    View* m_view;   //视图指针
    bool m_paused = false; //暂停标志
    QImage m_lastImage; // 保存最近一帧图像
    void saveImage();   // 截图保存函数
    Tcpserver* tcpWin = nullptr; // TCP服务器窗口指针
    DetectList* m_detectList = nullptr; // 对象检测列表窗口指针
    QSet<int> m_selectedObjectIds; // 当前选中的对象ID集合
    
    // 功能按钮状态管理
    void updateButtonDependencies(int clickedButtonId, bool isChecked);
}; 
