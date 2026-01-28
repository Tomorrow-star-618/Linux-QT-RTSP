# 数据库迁移问题修复说明

## 问题描述
点击方案预选功能时出现数据库错误，原因是旧版本数据库表结构与新版本不兼容。

## 问题原因
1. **旧表结构：** 没有 `camera_id` 字段，唯一约束为 `UNIQUE(name)`
2. **新表结构：** 增加了 `camera_id` 字段，唯一约束改为 `UNIQUE(camera_id, name)`
3. **CREATE IF NOT EXISTS 不会修改现有表：** 如果表已存在，该语句不会更新表结构

## 解决方案

### 自动迁移逻辑
修改 `createPlanTable()` 函数，增加智能检测和自动迁移：

```cpp
bool Plan::createPlanTable()
{
    QSqlQuery query(m_database);
    
    // 1. 检查表是否存在
    query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='plans'");
    bool tableExists = query.next();
    
    if (tableExists) {
        // 2. 检查是否有camera_id字段
        query.exec("PRAGMA table_info(plans)");
        bool hasCameraId = false;
        while (query.next()) {
            if (query.value(1).toString() == "camera_id") {
                hasCameraId = true;
                break;
            }
        }
        
        if (!hasCameraId) {
            // 3. 执行迁移流程
            // 3.1 重命名旧表
            query.exec("ALTER TABLE plans RENAME TO plans_old");
            
            // 3.2 创建新表（带camera_id字段）
            query.exec(createTableSql);
            
            // 3.3 迁移数据（camera_id默认为0）
            query.exec("INSERT INTO plans SELECT id, 0, name, ...");
            
            // 3.4 删除旧表
            query.exec("DROP TABLE plans_old");
            
            // 3.5 提示用户
            QMessageBox::information("数据库已自动升级！");
        }
    } else {
        // 表不存在，直接创建新表
        query.exec(createTableSql);
    }
}
```

## 迁移流程

### 步骤1：检测旧表
```sql
SELECT name FROM sqlite_master WHERE type='table' AND name='plans'
```

### 步骤2：检查字段
```sql
PRAGMA table_info(plans)
```
检查是否存在 `camera_id` 字段

### 步骤3：备份旧表
```sql
ALTER TABLE plans RENAME TO plans_old
```

### 步骤4：创建新表
```sql
CREATE TABLE plans (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    camera_id INTEGER DEFAULT 0,           -- [新增字段]
    name TEXT NOT NULL,
    rtsp_url TEXT NOT NULL,
    ai_enabled INTEGER DEFAULT 0,
    region_enabled INTEGER DEFAULT 0,
    object_enabled INTEGER DEFAULT 0,
    object_list TEXT DEFAULT '',
    created_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(camera_id, name)                -- [新约束]
)
```

### 步骤5：迁移数据
```sql
INSERT INTO plans (id, camera_id, name, rtsp_url, ai_enabled, region_enabled, object_enabled, object_list, created_time, updated_time)
SELECT id, 0, name, rtsp_url, ai_enabled, region_enabled, object_enabled, object_list, created_time, updated_time
FROM plans_old
```
**关键点：** 所有旧方案的 `camera_id` 设置为 0（主流）

### 步骤6：清理旧表
```sql
DROP TABLE plans_old
```

## 用户体验

### 首次启动（有旧数据）
1. 检测到旧版本数据库
2. 自动执行迁移（无需用户操作）
3. 显示提示信息：
   ```
   检测到旧版本数据库，已自动升级完成！
   所有旧方案已迁移为主流方案。
   ```
4. 方案列表正常显示，旧方案标记为 `[主流]`

### 首次启动（无旧数据）
1. 直接创建新表结构
2. 创建默认方案
3. 正常使用

### 后续使用
- 新表结构，无需再次迁移
- 支持多摄像头方案配置

## 错误处理

### 迁移失败场景
1. **重命名失败：** 显示错误信息，保留旧表
2. **创建新表失败：** 显示错误信息，可恢复旧表
3. **数据迁移失败：** 显示错误信息，保留新旧两表
4. **删除旧表失败：** 仅警告，不影响使用

### 错误恢复
如果迁移过程中出错，数据不会丢失：
- 旧表 `plans_old` 保留原始数据
- 可手动恢复或重新迁移

## 手动迁移步骤（如需）

如果自动迁移失败，可执行以下SQL：

```bash
# 进入数据库目录
cd ~/.local/share/<AppName>/

# 备份数据库
cp plans.db plans.db.backup

# 使用sqlite3工具
sqlite3 plans.db

# 执行迁移SQL
sqlite> ALTER TABLE plans RENAME TO plans_old;
sqlite> CREATE TABLE plans (
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
sqlite> INSERT INTO plans SELECT id, 0, name, rtsp_url, ai_enabled, region_enabled, object_enabled, object_list, created_time, updated_time FROM plans_old;
sqlite> DROP TABLE plans_old;
sqlite> .quit
```

## 测试验证

### 测试场景1：有旧数据
1. 删除新代码的可执行文件
2. 启动旧版本程序，创建几个方案
3. 关闭旧版本程序
4. 启动新版本程序
5. **预期结果：** 
   - 自动迁移成功提示
   - 旧方案显示为 `[主流]`
   - 可以创建新的子流方案

### 测试场景2：无旧数据
1. 删除 `~/.local/share/<AppName>/plans.db`
2. 启动新版本程序
3. **预期结果：**
   - 直接创建新表
   - 显示4个默认方案
   - 多摄像头标识正确

### 测试场景3：已迁移
1. 启动已迁移的程序
2. 再次启动
3. **预期结果：**
   - 不再执行迁移
   - 直接加载方案
   - 功能正常

## 日志输出

迁移过程中的调试日志：
```
检测到旧版本数据库表，开始迁移...
数据库迁移成功！
```

如果删除旧表失败：
```
警告：删除旧表失败，但不影响使用
```

## 注意事项

1. **数据安全：** 迁移前自动备份（重命名为plans_old）
2. **向后兼容：** 所有旧方案标记为主流（camera_id=0）
3. **无缝升级：** 用户无需手动操作
4. **错误提示：** 迁移失败时显示详细错误信息
5. **可恢复性：** 失败时可手动恢复或重试

## 影响文件
- `src/view/plan.cpp` - `createPlanTable()` 函数

## 版本信息
- 修复日期：2025-10-14
- 问题类型：数据库表结构不兼容
- 解决方案：自动检测和迁移
