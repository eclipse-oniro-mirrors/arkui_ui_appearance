# UI外观 API 知识库

> 更新时间：2026-07-30
> 源码路径：`interfaces/kits/`

## 概述

ui_appearance 仓提供三层 API 接口栈：Native C++ Kit、NAPI/JS Kit、ANI/ArkTS Kit。三层接口均委托给 `UiAppearanceAbilityClient`（IPC 客户端代理），最终通过 IPC 调用 `UiAppearanceAbility` SA 服务。

## 目录结构

```text
interfaces/
├── kits/
│   ├── native/                        # C++ Native Kit
│   │   ├── include/
│   │   │   ├── ui_appearance.h        # UIAppearance 类
│   │   │   └── ui_appearance_types.h  # DarkMode 枚举、错误码
│   │   └── src/
│   │       └── ui_appearance.cpp      # 委托到 UiAppearanceAbilityClient
│   └── napi/                          # NAPI/JS Kit
│       ├── include/
│       │   └── js_ui_appearance.h     # 异步上下文 + JsUiAppearance 类
│       └── src/
│           └── js_ui_appearance.cpp   # NAPI 模块注册
└── ets/
    └── ani/                           # ANI/ArkTS Kit
        ├── ets/
        │   └── @ohos.uiAppearance.ets # ArkTS 类型声明
        └── src/
            ├── ui_appearance.h        # ANI 函数声明
            └── ui_appearance.cpp      # ANI 绑定实现
```

## 核心类继承关系

```
UIAppearance (静态类)
  └── 委托到 UiAppearanceAbilityClient::GetInstance()

JsUiAppearance
  └── NAPI 模块注册，async work

ANI 绑定函数
  └── 委托到 UiAppearanceAbilityClient::GetInstance()
```

## 实现详解

### Native C++ Kit

**文件**：`interfaces/kits/native/include/ui_appearance.h`

**类**：`UIAppearance`（静态类，所有方法为 static）

**API**：

```cpp
static UiAppearanceAbilityErrCode SetDarkMode(DarkMode mode);
static UiAppearanceAbilityErrCode GetDarkMode(DarkMode& mode);
static UiAppearanceAbilityErrCode SetSettingData(std::string key, std::string value);
```

**类型定义**（`ui_appearance_types.h`）：

```cpp
enum DarkMode : int32_t {
    ALWAYS_DARK = 0,
    ALWAYS_LIGHT = 1,
    UNKNOWN = 2,
};

enum UiAppearanceAbilityErrCode : int32_t {
    SUCCEEDED = 0,
    PERMISSION_ERR = 201,
    NOT_SYSTEM_APP = 202,
    INVALID_ARG = 401,
    SYS_ERR = 500001,
};
```

**实现方式**：通过函数指针委托到 `UiAppearanceAbilityClient`，函数指针在 `ui_appearance.cpp` 中初始化。

**构建目标**：`ui_appearance_kit` → `libui_appearance_kit.z.so`（platformsdk innerapi）

### NAPI/JS Kit

**文件**：`interfaces/kits/napi/src/js_ui_appearance.cpp`

**模块名**：`@ohos.uiAppearance`

**API**：

| 方法 | 类型 | 说明 |
|---|---|---|
| `setDarkMode(mode: DarkMode, callback: AsyncCallback<void>)` | async | 设置深色模式（Callback） |
| `setDarkMode(mode: DarkMode): Promise<void>` | async | 设置深色模式（Promise） |
| `getDarkMode(): DarkMode` | sync | 获取当前深色模式 |
| `setFontScale(fontScale: string, callback: AsyncCallback<void>)` | async | 设置字体缩放（Callback） |
| `setFontScale(fontScale: string): Promise<void>` | async | 设置字体缩放（Promise） |
| `getFontScale(): string` | sync | 获取当前字体缩放 |
| `setFontWeightScale(fontWeightScale: string, callback: AsyncCallback<void>)` | async | 设置字体粗细缩放（Callback） |
| `setFontWeightScale(fontWeightScale: string): Promise<void>` | async | 设置字体粗细缩放（Promise） |
| `getFontWeightScale(): string` | sync | 获取当前字体粗细缩放 |

**DarkMode 枚举（JS）**：

```javascript
DarkMode {
    ALWAYS_DARK = 0,
    ALWAYS_LIGHT = 1,
}
```

**实现方式**：
- `set*` 方法使用 napi_async_work 实现
- `get*` 方法为同步调用
- 所有方法委托到 `UiAppearanceAbilityClient`

**构建目标**：`uiappearance` → `libuiappearance.so`

### ANI/ArkTS Kit

**文件**：`interfaces/ets/ani/src/ui_appearance.cpp`

**模块名**：`@ohos.uiAppearance.uiAppearance`

**API**：

| 方法 | 类型 | 说明 |
|---|---|---|
| `setDarkMode(mode: DarkMode, callback: AsyncCallback<void>)` | async | 设置深色模式（Callback） |
| `setDarkMode(mode: DarkMode): Promise<void>` | async | 设置深色模式（Promise） |
| `getDarkMode(): DarkMode` | sync | 获取当前深色模式 |
| `setFontScale(fontScale: string, callback: AsyncCallback<void>)` | async | 设置字体缩放（Callback） |
| `setFontScale(fontScale: string): Promise<void>` | async | 设置字体缩放（Promise） |
| `getFontScale(): string` | sync | 获取当前字体缩放 |
| `setFontWeightScale(fontWeightScale: string, callback: AsyncCallback<void>)` | async | 设置字体粗细缩放（Callback） |
| `setFontWeightScale(fontWeightScale: string): Promise<void>` | async | 设置字体粗细缩放（Promise） |
| `getFontWeightScale(): string` | sync | 获取当前字体粗细缩放 |

**实现方式**：
- 通过 ANI（ArkTS Native Interface）绑定
- 支持 Promise 和 Callback 两种异步模式
- 委托到 `UiAppearanceAbilityClient`

**构建目标**：`ui_appearance_ani` → `libui_appearance_ani.so` + `ui_appearance.abc`

## 完整 API 清单

### 三层接口对照表

| 功能 | Native C++ | NAPI/JS | ANI/ArkTS |
|---|---|---|---|
| 设置深色模式 | `SetDarkMode(DarkMode)` | `setDarkMode(mode)` | `setDarkMode(mode)` |
| 获取深色模式 | `GetDarkMode(DarkMode&)` | `getDarkMode()` | `getDarkMode()` |
| 设置字体缩放 | — | `setFontScale(fontScale)` | `setFontScale(fontScale)` |
| 获取字体缩放 | — | `getFontScale()` | `getFontScale()` |
| 设置字体粗细缩放 | — | `setFontWeightScale(fontWeightScale)` | `setFontWeightScale(fontWeightScale)` |
| 获取字体粗细缩放 | — | `getFontWeightScale()` | `getFontWeightScale()` |
| 设置通用数据 | `SetSettingData(string, string)` | — | — |

## 关键实现细节

### Native Kit 函数指针初始化

`UIAppearance` 类使用函数指针而非直接调用 `UiAppearanceAbilityClient`，允许在测试中注入 mock 实现：

```cpp
// ui_appearance.cpp
SetDarkModeFunc UIAppearance::setDarkModeFunc_ = [](DarkMode mode) -> UiAppearanceAbilityErrCode {
    auto client = UiAppearanceAbilityClient::GetInstance();
    return static_cast<UiAppearanceAbilityErrCode>(client->SetDarkMode(mode));
};
```

### NAPI 异步工作模型

`setDarkMode` / `setFontScale` / `setFontWeightScale` 使用 napi_async_work：
1. `Execute` 阶段：调用 `UiAppearanceAbilityClient` 的同步方法
2. `Complete` 阶段：根据结果创建 Promise resolve/reject 或调用 Callback

### ANI Promise/Callback 支持

`setDarkMode` 在 ANI 层支持两种调用方式：
- 如果传入 Callback，则使用 Callback 模式
- 如果未传入 Callback，则返回 Promise

## 使用示例

### Native C++

```cpp
#include "ui_appearance.h"
using namespace OHOS::ArkUi::UiAppearance;

auto errCode = UIAppearance::SetDarkMode(DarkMode::ALWAYS_DARK);
DarkMode mode;
errCode = UIAppearance::GetDarkMode(mode);
```

### NAPI/JS

```javascript
import uiAppearance from '@ohos.uiAppearance';

// Promise
uiAppearance.setDarkMode(uiAppearance.DarkMode.ALWAYS_DARK).then(() => {
    console.info('set dark mode succeeded');
});

// Callback
uiAppearance.setDarkMode(uiAppearance.DarkMode.ALWAYS_DARK, (err) => {
    if (err) {
        console.error('set dark mode failed: ' + err.code);
    }
});

// Sync get
let mode = uiAppearance.getDarkMode();
```

### ANI/ArkTS

```typescript
import uiAppearance from '@ohos.uiAppearance.uiAppearance';

// Promise
await uiAppearance.setDarkMode(uiAppearance.DarkMode.ALWAYS_DARK);

// Callback
uiAppearance.setDarkMode(uiAppearance.DarkMode.ALWAYS_DARK, (err) => {
    // ...
});

// Sync get
let mode = uiAppearance.getDarkMode();
```

## 调试指南

- 日志标签：`UiAppearance`
- 关键日志点：
  - NAPI/ANI 模块初始化
  - Set/Get 方法调用和返回值
  - 权限检查失败日志

## 常见问题

1. **Native Kit 链接错误**：确认 `libui_appearance_kit.z.so` 已正确链接
2. **NAPI 模块加载失败**：检查 `libuiappearance.so` 是否已安装到正确路径
3. **setDarkMode 返回 201**：缺少 `ohos.permission.UPDATE_CONFIGURATION` 权限
4. **setDarkMode 返回 401**：传入的 mode 值无效
5. **Native Kit 不支持 SetFontScale/SetFontWeightScale**：这两个接口仅通过 NAPI/ANI 暴露，Native Kit 不直接提供
