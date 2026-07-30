# 深色模式管理器知识库

> 更新时间：2026-07-30
> 源码路径：`services/src/dark_mode_manager.cpp`

## 概述

`DarkModeManager` 是 ui_appearance 仓中负责深色模式调度的核心管理器（单例），支持四种深色模式：始终浅色、始终深色、自定义定时、日出日落。它通过 `AlarmTimerManager` 管理定时调度，通过 `SettingDataManager` 监听设置数据变化，通过 `ScreenSwitchOperatorManager` 处理屏幕开关时的延迟切换，通过 `TemporaryColorModeManager` 管理临时颜色模式状态。

## 目录结构

```text
services/src/
├── dark_mode_manager.cpp              # 深色模式调度主实现
├── dark_mode_temp_state_manager.cpp   # 临时颜色模式状态管理
├── screen_switch_operator_manager.cpp # 屏幕开关状态追踪
services/utils/src/
├── alarm_timer_manager.cpp            # 定时器调度（TimeServiceClient）
├── alarm_timer.cpp                    # ITimerInfo 回调封装
```

## 核心类继承关系

```
NoCopyable
  └── DarkModeManager (单例)
        ├── AlarmTimerManager (定时器管理)
        ├── TemporaryColorModeManager (临时颜色模式)
        └── ScreenSwitchOperatorManager (屏幕开关状态)

NoCopyable
  └── SettingDataManager (单例，DataShare CRUD)
```

## 实现详解

### 深色模式模式

| 模式 | 枚举值 | 行为 |
|---|---|---|
| `DARK_MODE_ALWAYS_LIGHT` | 0 | 始终浅色；清除定时器 |
| `DARK_MODE_ALWAYS_DARK` | 1 | 始终深色；清除定时器 |
| `DARK_MODE_CUSTOM_AUTO` | 2 | 自定义时间范围；通过 `settings.uiappearance.darkmode_starttime/endtime` 设置 |
| `DARK_MODE_SUNRISE_SUNSET` | 3 | 日出日落；通过 `settings.display.sun_set/sun_rise` 设置 |

### 初始化流程

1. `Initialize(updateCallback)` — 保存回调函数
2. `LoadUserSettingData(context, needUpdateCallback, isDarkMode, bootLoadFlag)` — 加载用户设置数据
   - 从 DataShare 读取 `settings.uiappearance.darkmode_mode`
   - 根据模式读取 starttime/endtime 或 sunset/sunrise
   - 调用 `OnStateChangeLocked` 计算当前是否应处于深色模式
   - 注册 DataShare 观察者

### 调度逻辑

1. **始终浅色/深色模式**：直接切换，清除定时器
2. **自定义定时模式**：
   - 读取 `settings.uiappearance.darkmode_starttime` 和 `settings.uiappearance.darkmode_endtime`（分钟，从午夜起算）
   - 通过 `AlarmTimerManager` 创建两个每日重复定时器
   - 定时器触发时，检查当前时间是否在深色区间内
3. **日出日落模式**：
   - 读取 `settings.display.sun_set` 和 `settings.display.sun_rise`（分钟，从午夜起算；sunrise > 1440 表示次日）
   - 同样通过 `AlarmTimerManager` 创建定时器
   - 默认值：sunset=18:00 (1080), sunrise=次日7:00 (1860)

### 定时器触发回调

```
Timer Start Callback → CheckTimerCallbackParams → 判断是否在深色区间
  → 如果在深色区间：切换到深色
  → 如果不在深色区间：切换到浅色

Timer End Callback → CheckTimerCallbackParams → 判断是否离开深色区间
  → 切换到浅色
```

### 屏幕开关延迟切换

当屏幕关闭时，深色模式的切换被延迟到屏幕打开时执行：
- `ScreenOffCallback` → 记录待切换操作
- `ScreenOnCallback` → 执行延迟的切换操作

### 临时颜色模式

`TemporaryColorModeManager` 管理临时颜色模式状态：
- 持久化到系统参数：`persist.uiAppearance.dark_mode_temp_state_flag.<ctx>`
- 跟踪时间窗口：`persist.uiAppearance.dark_mode_temp_state_start_time.<ctx>` / `end_time.<ctx>`
- `IsColorModeTemporary` — 检查是否处于临时模式
- `CheckTemporaryStateEffective` — 检查临时状态是否仍在有效时间窗口内

### 设置数据观察者

DarkModeManager 注册 5 个 DataShare 观察者：

| Key | 观察者回调 |
|---|---|
| `settings.uiappearance.darkmode_mode` | `SettingDataDarkModeModeUpdateFunc` |
| `settings.uiappearance.darkmode_starttime` | `SettingDataDarkModeStartTimeUpdateFunc` |
| `settings.uiappearance.darkmode_endtime` | `SettingDataDarkModeEndTimeUpdateFunc` |
| `settings.display.sun_set` | `SettingDataDarkModeSunsetTimeUpdateFunc` |
| `settings.display.sun_rise` | `SettingDataDarkModeSunriseTimeUpdateFunc` |

### 时间/时区变化处理

`COMMON_EVENT_TIME_CHANGED` / `COMMON_EVENT_TIMEZONE_CHANGED` → `RestartTimer()`：
1. 重新计算所有用户的定时器触发时间
2. 通过 `AlarmTimerManager::RestartAllTimer()` 更新定时器

## 完整 API 清单

```cpp
// DarkModeManager 公开接口
static DarkModeManager& GetInstance();
ErrCode Initialize(const std::function<void(bool, int32_t)>& updateCallback);
ErrCode LoadUserSettingData(int32_t userId, bool needUpdateCallback, bool& isDarkMode, bool bootLoadFlag);
ErrCode LoadUserSettingData(const AccountContext& context, bool needUpdateCallback, bool& isDarkMode, bool bootLoadFlag);
void NotifyDarkModeUpdate(int32_t userId, bool isDarkMode);
void NotifyDarkModeUpdate(const AccountContext& context, bool isDarkMode);
ErrCode OnSwitchUser(int32_t userId);
ErrCode OnSwitchContext(const AccountContext& context);
void ScreenOnCallback();
void ScreenOffCallback();
ErrCode RestartTimer();
bool GetSettingTime(const AccountContext& context, int32_t& settingStartTime, int32_t& settingEndTime);
bool IsColorModeNormal(const AccountContext& context);
void DoSwitchTemporaryColorMode(const AccountContext& context, bool isDarkMode);
static ErrCode GetCurrentTimeOfSeconds(int32_t& seconds);
static bool CheckCurrentTimeInDarkInterval(int32_t startTime, int32_t endTime, int32_t seconds);
```

## 关键实现细节

### 深色区间判断

`CheckCurrentTimeInDarkInterval(startTime, endTime, seconds)`：
- startTime 和 endTime 为分钟数（从午夜起算）
- 如果 endTime > 1440（次日），则跨日计算
- 如果 startTime < endTime（同日），则 seconds 在 [startTime, endTime) 内为深色
- 如果 startTime >= endTime（跨日），则 seconds 在 [startTime, 1440) 或 [0, endTime) 内为深色

### 定时器触发时间计算

`AlarmTimerManager::SetTimerTriggerTime`：
- 计算当前时间到下一次触发时间的毫秒偏移
- 如果今天的触发时间已过，则设置为明天的触发时间
- 两个定时器：startTimer（切换到深色）和 endTimer（切换到浅色）

### 用户切换

`OnSwitchUser` / `OnSwitchContext`：
1. 反注册当前用户的 DataShare 观察者
2. 注册新用户的 DataShare 观察者
3. 加载新用户的设置数据

## 使用示例

```cpp
// 初始化（在 UiAppearanceAbility::Init 中）
DarkModeManager::GetInstance().Initialize(
    [this](bool isDarkMode, int32_t userId) {
        UpdateDarkModeCallback(isDarkMode, userId);
    }
);

// 加载用户设置
bool isDarkMode = false;
DarkModeManager::GetInstance().LoadUserSettingData(userId, true, isDarkMode, true);

// 屏幕事件
DarkModeManager::GetInstance().ScreenOnCallback();
DarkModeManager::GetInstance().ScreenOffCallback();
```

## 调试指南

- 日志标签：`UiAppearance`（与 SA 共用）
- 关键日志点：
  - `LoadUserSettingData`：读取的模式值和计算结果
  - `OnStateChangeLocked`：模式切换决策
  - `CreateOrUpdateTimers`：定时器创建/更新
  - `ScreenOnCallback` / `ScreenOffCallback`：屏幕事件处理
  - `CheckTimerCallbackParams`：定时器回调参数验证

## 常见问题

1. **自定义定时模式不生效**：检查 DataShare 中 `darkmode_starttime` 和 `darkmode_endtime` 的值是否有效（0-2880 范围内的分钟数）
2. **日出日落模式使用默认值**：DataShare 中未设置 `sun_set`/`sun_rise` 时使用默认值 18:00 / 次日7:00
3. **定时器未触发**：检查 `AlarmTimerManager` 是否正确初始化，TimeService 是否可用
4. **屏幕关闭后切换不生效**：`ScreenSwitchOperatorManager` 会在屏幕打开时执行延迟切换，确认屏幕事件是否正确传递
5. **临时颜色模式不恢复**：检查 `TemporaryColorModeManager` 的有效时间窗口是否正确持久化
