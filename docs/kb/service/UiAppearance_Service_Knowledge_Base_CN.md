# UiAppearance SA 服务知识库

> 更新时间：2026-07-30
> 源码路径：`services/src/ui_appearance_ability.cpp`

## 概述

`UiAppearanceAbility` 是 ui_appearance 仓的核心 SA（System Ability）服务类，SA ID 为 7002，运行在 `ui_service` 进程中。它负责管理系统的 UI 外观设置，包括深色模式、字体缩放、字体粗细缩放，以及通用设置数据存储。服务通过 IDL 生成的 Stub/Proxy 提供 IPC 接口，支持多用户和子配置（车机平台）。

## 目录结构

```text
services/
├── IUiAppearanceAbility.idl          # IDL 接口定义
├── BUILD.gn                          # 构建目标
├── include/
│   ├── ui_appearance_ability.h       # 主 SA 类
│   ├── ui_appearance_ability_client.h # IPC 客户端代理
│   ├── dark_mode_manager.h           # 深色模式调度
│   ├── dark_mode_temp_state_manager.h # 临时颜色模式
│   ├── smart_gesture_manager.h       # 智慧手势
│   ├── account_context.h             # 用户/子配置上下文
│   ├── screen_switch_operator_manager.h # 屏幕开关状态
│   ├── background_app_color_switch_settings.h # 后台应用颜色切换白名单
│   └── ui_appearance_log.h           # 日志宏
├── src/
│   ├── ui_appearance_ability.cpp     # 主 SA 实现
│   ├── ui_appearance_ability_client.cpp # IPC 客户端实现
│   ├── dark_mode_manager.cpp         # 深色模式调度实现
│   ├── dark_mode_temp_state_manager.cpp # 临时颜色模式实现
│   ├── smart_gesture_manager.cpp     # 智慧手势实现
│   ├── account_context.cpp           # 用户上下文实现
│   ├── screen_switch_operator_manager.cpp # 屏幕状态实现
│   └── background_app_color_switch_settings.cpp # 白名单实现
└── utils/
    ├── include/
    │   ├── setting_data_manager.h    # DataShare CRUD + 观察者
    │   ├── setting_data_observer.h   # 数据变化观察者
    │   ├── alarm_timer_manager.h     # 定时器调度
    │   ├── alarm_timer.h             # ITimerInfo 封装
    │   ├── parameter_wrap.h          # GetParameter/SetParameter 封装
    │   ├── json_utils.h              # JSON 文件加载
    │   └── ipc_skeleton_utils.h      # RAII ResetCallingIdentity
    └── src/
        ├── setting_data_manager.cpp
        ├── setting_data_observer.cpp
        ├── alarm_timer_manager.cpp
        ├── alarm_timer.cpp
        ├── parameter_wrap.cpp
        └── json_utils.cpp
```

## 核心类继承关系

```
SystemAbility
  └── UiAppearanceAbility
        └── UiAppearanceAbilityStub (IDL 生成)
              └── IUiAppearanceAbility (IDL 接口)

IRemoteObject::DeathRecipient
  └── UiAppearanceDeathRecipient

EventFwk::CommonEventSubscriber
  └── UiAppearanceEventSubscriber
```

## 实现详解

### SA 生命周期

1. **OnStart()**：发布 SA 到 SAMGR，监听 `APP_MGR_SERVICE_ID`
2. **OnAddSystemAbility(APP_MGR_SERVICE_ID)**：
   - 检查首次升级兼容性（`DoCompatibleProcess`）
   - 初始化 `DarkModeManager`（传入深色模式更新回调）
   - 初始化 `SmartGestureManager`（传入手势更新回调）
   - 订阅公共事件（用户切换、时间变化、屏幕开关、开机完成）
   - 执行兼容性迁移和每用户参数初始化
3. **OnStop()**：日志记录

### IPC 接口实现

| 方法 | 权限 | 行为 |
|---|---|---|
| `SetDarkMode(mode)` | `ohos.permission.UPDATE_CONFIGURATION` | 设置深色模式，更新 Configuration，持久化参数 |
| `GetDarkMode()` | 无 | 从内存参数表读取当前深色模式 |
| `SetFontScale(fontScale)` | `ohos.permission.UPDATE_CONFIGURATION` | 设置字体缩放，更新 Configuration，持久化参数 |
| `GetFontScale(fontScale)` | 无 | 从内存参数表读取字体缩放 |
| `SetFontWeightScale(fontWeightScale)` | `ohos.permission.UPDATE_CONFIGURATION` | 设置字体粗细缩放，更新 Configuration，持久化参数 |
| `GetFontWeightScale(fontWeightScale)` | 无 | 从内存参数表读取字体粗细缩放 |
| `SetSettingData(key, value)` | 系统应用 | 通过 DataShare 写入通用设置数据 |

### 多用户支持

- 每个用户维护独立的 `UiAppearanceParam`（darkMode, fontScale, fontWeightScale）
- 通过 `AccountContext`（userId + subProfileId）索引用户参数
- 用户切换时重新加载设置数据、重注册观察者、更新 Configuration
- 子配置切换（车机平台）时将中心用户外观应用到目标子配置

### 公共事件处理

| 事件 | 响应 |
|---|---|
| `COMMON_EVENT_BOOT_COMPLETED` | 初始化深色模式 + 智慧手势 |
| `COMMON_EVENT_USER_SWITCHED` | 切换用户上下文，重新加载设置 |
| `COMMON_EVENT_TIME_CHANGED` | 重启深色模式定时器 |
| `COMMON_EVENT_TIMEZONE_CHANGED` | 重启深色模式定时器 |
| `COMMON_EVENT_SCREEN_ON` | 通知 DarkModeManager（执行延迟切换） |
| `COMMON_EVENT_SCREEN_OFF` | 通知 DarkModeManager（排队延迟切换） |
| `COMMON_EVENT_OS_ACCOUNT_SUB_PROFILE_SWITCHED` | 应用中心外观到子配置（车机） |

## 完整 API 清单

### IDL 接口 (IUiAppearanceAbility)

```cpp
int SetDarkMode([in] int darkMode);
int GetDarkMode();
int GetFontScale([out] String fontScale);
int SetFontScale([in] String fontScale);
int GetFontWeightScale([out] String fontWeightScale);
int SetFontWeightScale([in] String fontWeightScale);
int SetSettingData([in] String key, [in] String value);
```

### Native C++ Kit (UIAppearance)

```cpp
static UiAppearanceAbilityErrCode SetDarkMode(DarkMode mode);
static UiAppearanceAbilityErrCode GetDarkMode(DarkMode& mode);
static UiAppearanceAbilityErrCode SetSettingData(std::string key, std::string value);
```

## 关键实现细节

### Configuration 更新流程

`SetDarkMode` → `OnSetDarkMode` → `UpdateCurrentUserConfiguration` → `UpdateConfiguration` → AppMgr::UpdateConfiguration → 持久化参数

### 深色模式回调

`DarkModeManager` 在定时器触发时回调 `UpdateDarkModeCallback`，该回调负责：
1. 更新内存中的 `UiAppearanceParam`
2. 调用 `UpdateConfiguration` 更新系统配置
3. 持久化参数到系统参数

### 后台应用颜色切换

`BackGroundAppColorSwitchSettings` 从 `/etc/dark_mode_whilelist.json` 加载白名单，在深色模式切换时批量更新后台应用的颜色配置。

## 使用示例

```cpp
// Native C++ Kit
#include "ui_appearance.h"
using namespace OHOS::ArkUi::UiAppearance;

auto errCode = UIAppearance::SetDarkMode(DarkMode::ALWAYS_DARK);
DarkMode mode;
errCode = UIAppearance::GetDarkMode(mode);
```

```javascript
// NAPI/JS
import uiAppearance from '@ohos.uiAppearance';
uiAppearance.setDarkMode(uiAppearance.DarkMode.ALWAYS_DARK);
let mode = uiAppearance.getDarkMode();
```

## 调试指南

- 日志标签：`UiAppearance`
- 日志域：`0xD003900`
- 关键日志点：
  - `OnStart` / `OnStop` 生命周期
  - `SetDarkMode` / `SetFontScale` / `SetFontWeightScale` 调用
  - `UpdateConfiguration` 结果
  - 用户切换事件处理

## 常见问题

1. **SetDarkMode 返回 201**：缺少 `ohos.permission.UPDATE_CONFIGURATION` 权限
2. **SetSettingData 返回 202**：调用方不是系统应用
3. **SetDarkMode 返回 401**：传入的 mode 值无效（不在 0-3 范围内）
4. **深色模式定时切换不生效**：检查 DataShare 中的 `settings.uiappearance.darkmode_mode` 是否为 2 或 3，以及对应的 starttime/endtime 或 sunrise/sunset 值
5. **多用户场景下外观不生效**：确认用户切换事件是否正确处理，检查 `AccountContext` 是否正确构建
