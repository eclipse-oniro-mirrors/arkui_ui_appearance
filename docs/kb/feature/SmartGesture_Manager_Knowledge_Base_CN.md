# 智慧手势管理器知识库

> 更新时间：2026-07-30
> 源码路径：`services/src/smart_gesture_manager.cpp`

## 概述

`SmartGestureManager` 是 ui_appearance 仓中负责智慧手势模式管理的单例管理器。它支持两种模式：禁用（`SMART_GESTURE_DISABLE`）和自动（`SMART_GESTURE_AUTO`），通过 `SettingDataManager` 监听 `persist.gesture.smart_gesture_enable` 设置数据的变化，并在模式变化时通知上层回调。

## 目录结构

```text
services/src/
├── smart_gesture_manager.cpp          # 智慧手势管理实现
services/include/
├── smart_gesture_manager.h            # 智慧手势管理头文件
```

## 核心类继承关系

```
NoCopyable
  └── SmartGestureManager (单例)
```

## 实现详解

### 模式定义

| 模式 | 枚举值 | 行为 |
|---|---|---|
| `SMART_GESTURE_DISABLE` | 0 | 禁用智慧手势 |
| `SMART_GESTURE_AUTO` | 1 | 自动模式（开启智慧手势） |

### 初始化流程

1. `Initialize(updateCallback)` — 保存回调函数
2. `RegisterSettingDataObserver()` — 注册 DataShare 观察者监听 `persist.gesture.smart_gesture_enable`
3. `UpdateSmartGestureInitialValue()` — 从 DataShare 读取当前值并更新

### 设置数据观察者

| Key | 观察者回调 |
|---|---|
| `persist.gesture.smart_gesture_enable` | `LoadSettingDataObserversCallback` |

### 模式变化回调

当设置数据变化时：
1. `LoadSettingDataObserversCallback` 被调用
2. 读取新的 `smart_gesture_enable` 值
3. `OnChangeSmartGestureMode` 更新内部状态
4. 调用 `updateCallback_(isAutoMode, userId)` 通知上层

### 上层回调处理

`UiAppearanceAbility::UpdateSmartGestureModeCallback(isAutoMode, userId)`：
1. 更新 Configuration 中的 `smartGestureMode` 字段
2. 通过 AppMgr 更新系统配置

## 完整 API 清单

```cpp
// SmartGestureManager 公开接口
static SmartGestureManager& GetInstance();
ErrCode Initialize(const std::function<void(bool, int32_t)>& updateCallback);
ErrCode RegisterSettingDataObserver() const;
void UpdateSmartGestureInitialValue();
```

## 关键实现细节

### 设置数据读取

- Key：`persist.gesture.smart_gesture_enable`
- 类型：int32_t
- 值：0（禁用）或 1（自动）
- 通过 `SettingDataManager::GetInt32Value` 读取

### 回调机制

`SmartGestureManager` 不直接管理 Configuration 更新，而是通过回调通知 `UiAppearanceAbility`：
- `updateCallback_(true, userId)` — 自动模式
- `updateCallback_(false, userId)` — 禁用模式

## 使用示例

```cpp
// 初始化（在 UiAppearanceAbility::Init 中）
SmartGestureManager::GetInstance().Initialize(
    [this](bool isAutoMode, int32_t userId) {
        UpdateSmartGestureModeCallback(isAutoMode, userId);
    }
);

// 注册观察者
SmartGestureManager::GetInstance().RegisterSettingDataObserver();

// 初始化值
SmartGestureManager::GetInstance().UpdateSmartGestureInitialValue();
```

## 调试指南

- 日志标签：`UiAppearance`
- 关键日志点：
  - `LoadSettingDataObserversCallback`：设置数据变化回调
  - `OnChangeSmartGestureMode`：模式切换
  - `RegisterSettingDataObserver`：观察者注册结果

## 常见问题

1. **智慧手势模式不生效**：检查 DataShare 中 `persist.gesture.smart_gesture_enable` 的值是否为 0 或 1
2. **观察者注册失败**：检查 `SettingDataManager` 是否已初始化，DataShare 服务是否可用
3. **模式切换后 Configuration 未更新**：确认 `updateCallback` 是否正确绑定到 `UiAppearanceAbility::UpdateSmartGestureModeCallback`
