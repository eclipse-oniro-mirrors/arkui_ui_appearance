# Dark Mode Manager Context

> 文档版本：v1.0
> 更新时间：2026-07-30
> 来源：`docs/context_registry.json` 主题 `DarkModeManager`

## 定位

DarkModeManager 是 ui_appearance 仓中负责深色模式调度的核心管理器（单例），支持四种深色模式：始终浅色、始终深色、自定义定时、日出日落。它通过 AlarmTimerManager 管理定时调度，通过 SettingDataManager 监听设置数据变化，通过 ScreenSwitchOperatorManager 处理屏幕开关时的延迟切换，通过 TemporaryColorModeManager 管理临时颜色模式状态。

本文档只提供稳定的源码、API、测试和 Spec 路由。具体行为、默认值、边界条件和兼容性说明以对应源码实现、测试用例和 Spec 为准。

## 快速路由

### 源码入口

| 关注点 | 稳定路径 | 说明 |
|--------|----------|------|
| 调度主类 | `services/src/dark_mode_manager.cpp` | `DarkModeManager`：4 种模式调度、定时器管理、DataShare 观察者、用户切换 |
| 调度头文件 | `services/include/dark_mode_manager.h` | `DarkModeMode` 枚举、`DarkModeState` 结构、公开/私有接口声明 |
| 临时颜色模式 | `services/src/dark_mode_temp_state_manager.cpp` | `TemporaryColorModeManager`：临时状态持久化、时间窗口校验 |
| 临时颜色头文件 | `services/include/dark_mode_temp_state_manager.h` | `TempColorModeType`、`TempColorModeInfo` 结构 |
| 屏幕开关状态 | `services/src/screen_switch_operator_manager.cpp` | `ScreenSwitchOperatorManager`：屏幕关闭时延迟切换排队 |
| 屏幕开关头文件 | `services/include/screen_switch_operator_manager.h` | `ScreenSwitchType`、`ScreenOffOperateType` 枚举 |
| 定时器管理 | `services/utils/src/alarm_timer_manager.cpp` | `AlarmTimerManager`：TimeServiceClient 定时器创建/更新/重启 |
| 定时器头文件 | `services/utils/include/alarm_timer_manager.h` | `SetScheduleTime`、`ClearTimerByUserId`、`RestartAllTimer` |

### 模式定义

| 模式 | 枚举值 | DataShare Key | 说明 |
|------|--------|---------------|------|
| `DARK_MODE_ALWAYS_LIGHT` | 0 | — | 始终浅色，清除定时器 |
| `DARK_MODE_ALWAYS_DARK` | 1 | — | 始终深色，清除定时器 |
| `DARK_MODE_CUSTOM_AUTO` | 2 | `settings.uiappearance.darkmode_starttime/endtime` | 自定义时间范围（分钟，从午夜起算） |
| `DARK_MODE_SUNRISE_SUNSET` | 3 | `settings.display.sun_set/sun_rise` | 日出日落模式（sunrise > 1440 表示次日） |

### 测试入口

| 稳定路径 | 用途 |
|----------|------|
| `test/unittest/dark_mode_manager_test/dark_mode_manager_test.cpp` | 深色模式调度测试：初始化、LoadUserSettingData、4 种模式、定时器回调、用户切换、屏幕开关 |

### 相关主题

| 主题 | 路由 |
|------|------|
| UI外观服务 | `docs/kb/service/ui-appearance-service.md` |
| 架构设计 | `docs/kb/architecture/ui-appearance-architecture.md` |

## 常见问题定位

| 问题 | 优先查看 |
|------|----------|
| 自定义定时模式不生效 | `settings.uiappearance.darkmode_starttime/endtime` 值是否有效；`OnStateChangeToCustomAutoMode` |
| 日出日落模式使用默认值 | `settings.display.sun_set/sun_rise` 未设置时默认 sunset=1080(18:00)、sunrise=1860(次日7:00) |
| 定时器未触发 | `AlarmTimerManager::SetScheduleTime`、TimeService 可用性 |
| 屏幕关闭后切换不生效 | `ScreenSwitchOperatorManager`：`ScreenOffCallback` 排队、`ScreenOnCallback` 执行延迟切换 |
| 临时颜色模式不恢复 | `TemporaryColorModeManager::CheckTemporaryStateEffective`、时间窗口持久化参数 |
| 时间/时区变化后定时器未更新 | `RestartTimer` → `AlarmTimerManager::RestartAllTimer` |

## 调试入口

- 日志标签：`UiAppearance`
- 关键方法：`LoadUserSettingData`、`OnStateChangeLocked`、`CreateOrUpdateTimers`、`CheckTimerCallbackParams`
- 系统参数：`param get persist.ace.darkmode`
- DataShare：`settings get settings.uiappearance.darkmode_mode`
