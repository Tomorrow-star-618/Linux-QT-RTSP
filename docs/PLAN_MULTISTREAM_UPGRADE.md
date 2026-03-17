# 方案预选功能多路适配升级说明

## 概述
将方案预选（Plan）功能从单路适配升级为多路支持，允许为每个摄像头通道（主流/子流）配置独立的监控方案。

## 修改文件列表
1. `src/view/plan.h` - 头文件
2. `src/view/plan.cpp` - 实现文件
3. `src/controller/controller.cpp` - 控制器适配

---

## 详细修改内容

### 1. 数据结构升级 (plan.h)

#### PlanData结构体
```cpp
struct PlanData {
    int id;                    // 方案ID（数据库主键）
    int cameraId;              // [新增] 摄像头ID（0=主流，1-N=子流）
    QString name;              // 方案名称
    QString rtspUrl;           // RTSP地址
    bool aiEnabled;            // AI识别功能使能
    bool regionEnabled;        // 区域识别功能使能
    bool objectEnabled;        // 对象识别功能使能
    QSet<int> objectList;      // 对象列表（对象ID集合）
    
    // 默认构造函数
    PlanData() : id(-1), cameraId(0), aiEnabled(false), regionEnabled(false), objectEnabled(false) {}
};
```

**变更说明：**
- 新增 `cameraId` 字段，用于标识方案所属的摄像头
- cameraId=0 表示主流，1-N 表示对应的子流

#### UI控件新增
```cpp
QComboBox* m_cameraIdComboBox;  // 摄像头ID选择（0=主流，1-N=子流）
```

---

### 2. 数据库表结构升级 (plan.cpp)

#### 新表结构
```sql
CREATE TABLE IF NOT EXISTS plans (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    camera_id INTEGER DEFAULT 0,           -- [新增] 摄像头ID
    name TEXT NOT NULL,
    rtsp_url TEXT NOT NULL,
    ai_enabled INTEGER DEFAULT 0,
    region_enabled INTEGER DEFAULT 0,
    object_enabled INTEGER DEFAULT 0,
    object_list TEXT DEFAULT '',
    created_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(camera_id, name)                -- [变更] 组合唯一约束
)
```

**关键变更：**
- 新增 `camera_id` 字段
- 唯一约束从 `name` 改为 `(camera_id, name)` 组合约束
  - 允许不同摄像头使用相同的方案名称
  - 同一摄像头下方案名称必须唯一

---

### 3. UI界面升级

#### 摄像头选择控件
- **位置：** 方案配置表单的第一项
- **内容：** 
  - 主流（ID: 0）
  - 子流1-16（ID: 1-16）
- **样式：** 蓝色主题，与系统整体风格一致

#### 方案列表显示
- **主流方案：** `[主流] 方案名称` （蓝色文字）
- **子流方案：** `[Cam1] 方案名称` （绿色文字）

---

### 4. 数据库操作适配

#### loadPlansFromDatabase()
```sql
-- 查询语句增加camera_id字段，并按摄像头ID排序
SELECT id, camera_id, name, rtsp_url, ai_enabled, region_enabled, object_enabled, object_list 
FROM plans 
ORDER BY camera_id, id
```

#### savePlanToDatabase()
```sql
-- INSERT语句增加camera_id字段
INSERT INTO plans (camera_id, name, rtsp_url, ai_enabled, region_enabled, object_enabled, object_list) 
VALUES (?, ?, ?, ?, ?, ?, ?)

-- UPDATE语句增加camera_id字段
UPDATE plans 
SET camera_id=?, name=?, rtsp_url=?, ai_enabled=?, region_enabled=?, object_enabled=?, object_list=?, updated_time=CURRENT_TIMESTAMP 
WHERE id=?
```

---

### 5. 表单处理升级

#### updateFormFromPlan()
- 新增摄像头ID下拉框的数据绑定
- 自动阻塞和恢复信号以避免循环触发

#### updatePlanFromForm()
- 从ComboBox的userData获取摄像头ID
- 确保数据类型正确（int）

#### 验证逻辑增强
- 同一摄像头下方案名称唯一性检查
- 错误提示包含摄像头信息

---

### 6. 默认方案升级 (createDefaultPlans)

```cpp
// 主流方案示例
PlanData plan1;
plan1.cameraId = 0;  // 主流
plan1.name = "基础监控方案";
// ...

// 子流1方案示例
PlanData plan3;
plan3.cameraId = 1;  // 子流1
plan3.name = "区域监控方案";
// ...
```

**默认方案配置：**
- 主流：基础监控方案、全功能监控方案
- 子流1：区域监控方案
- 子流2：简单监控方案

---

### 7. Controller适配 (controller.cpp)

#### onPlanApplied() 函数升级

**关键变更：**
```cpp
// [旧] 使用TCP当前绑定的摄像头ID
int targetCameraId = tcpWin->getCurrentCameraId();

// [新] 使用方案中指定的摄像头ID
int targetCameraId = plan.cameraId;
QString cameraName = (targetCameraId == 0) ? "主流" : QString("子流%1").arg(targetCameraId);
```

**日志输出优化：**
- 所有操作日志都包含摄像头标识 `[主流]` 或 `[子流N]`
- 便于多路调试和问题追踪

**示例日志：**
```
[主流] 已设置RTSP地址: rtsp://192.168.1.130/live/0
[主流] AI功能已启用
[子流1] 区域识别功能已启用
[子流2] 已设置检测对象列表(1个): person
```

---

### 8. 应用确认对话框升级

**新对话框内容：**
```
确定要应用方案 "基础监控方案" 到 [主流] 吗？
这将会：
• 目标摄像头：主流（ID: 0）
• 设置RTSP地址：rtsp://192.168.1.130/live/0
• 配置AI功能：启用
• 配置区域识别：禁用
• 配置对象识别：启用
• 设置检测对象列表
```

---

## 数据库迁移说明

### 自动处理
- SQLite的 `CREATE TABLE IF NOT EXISTS` 语句确保向后兼容
- 旧数据库会自动添加 `camera_id` 字段（默认值为0，即主流）
- 新表结构会自动应用组合唯一约束

### 手动迁移（如需）
如果遇到唯一约束冲突，可执行以下SQL：
```sql
-- 备份旧表
ALTER TABLE plans RENAME TO plans_old;

-- 创建新表
CREATE TABLE plans (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    camera_id INTEGER DEFAULT 0,
    name TEXT NOT NULL,
    rtsp_url TEXT NOT NULL,
    ai_enabled INTEGER DEFAULT 0,
    region_enabled INTEGER DEFAULT 0,
    object_enabled INTEGER DEFAULT 0,
    object_list TEXT DEFAULT '',
    created_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(camera_id, name)
);

-- 迁移数据（所有旧方案默认为主流）
INSERT INTO plans (id, camera_id, name, rtsp_url, ai_enabled, region_enabled, object_enabled, object_list, created_time, updated_time)
SELECT id, 0, name, rtsp_url, ai_enabled, region_enabled, object_enabled, object_list, created_time, updated_time
FROM plans_old;

-- 删除旧表
DROP TABLE plans_old;
```

---

## 使用说明

### 创建多路方案
1. 点击"新建"按钮
2. 在"目标摄像头"下拉框选择要配置的摄像头
3. 输入方案名称（同一摄像头下不能重复）
4. 配置RTSP地址、AI功能、区域识别、对象识别等
5. 点击"保存方案"

### 应用方案到指定摄像头
1. 在方案列表中选择要应用的方案
2. 点击"应用方案"按钮
3. 确认对话框中会显示目标摄像头信息
4. 确认后，系统会将配置发送到指定的摄像头通道

### 方案命名建议
- **主流方案：** "办公区域监控"、"入口全景监控"等
- **子流方案：** "办公区域-人员检测"、"入口-车辆识别"等

---

## 优势与特性

### 1. 多路独立配置
- 每个摄像头通道可以有不同的监控策略
- 主流用于全景监控，子流用于特定目标检测

### 2. 方案复用
- 不同摄像头可以使用同名方案（配置不同）
- 便于统一管理和批量配置

### 3. 灵活切换
- 快速在多个预设方案间切换
- 减少重复配置工作

### 4. 可视化管理
- 方案列表直观显示摄像头归属
- 颜色标识区分主流和子流

### 5. 安全验证
- 组合唯一约束防止配置冲突
- 详细的确认对话框避免误操作

---

## 测试建议

### 单元测试
1. 创建主流方案并保存
2. 创建子流方案并保存
3. 验证同名方案在不同摄像头下可以共存
4. 验证同一摄像头下方案名称唯一性

### 集成测试
1. 创建多个摄像头的方案
2. 依次应用到对应摄像头，验证TCP命令发送
3. 检查日志输出是否包含正确的摄像头标识
4. 验证方案切换后配置是否正确应用

### 数据库测试
1. 检查新建方案后camera_id字段是否正确
2. 验证唯一约束是否生效
3. 测试数据库迁移兼容性

---

## 注意事项

1. **数据库文件位置：** `~/.local/share/<AppName>/plans.db`
2. **默认摄像头ID：** 0（主流）
3. **最大子流数量：** 16个（可根据需要调整）
4. **TCP连接要求：** 应用方案前需确保TCP连接已建立
5. **方案应用目标：** 以方案中的cameraId为准，不受UI当前选择影响

---

## 后续优化建议

1. **批量应用：** 支持同时应用多个方案到不同摄像头
2. **方案导入导出：** JSON格式导入导出方案配置
3. **方案模板：** 预定义常用场景模板
4. **历史记录：** 记录方案应用历史
5. **性能优化：** 大量方案时的加载优化
6. **分组管理：** 按摄像头分组显示方案

---

## 版本信息
- **升级日期：** 2025-10-14
- **影响范围：** Plan模块、Controller模块
- **兼容性：** 向后兼容，旧数据自动迁移为主流方案
