# UiAppearance API Context

> 文档版本：v1.0
> 更新时间：2026-07-30
> 来源：`docs/context_registry.json` 主题 `UiAppearanceAPI`

## 定位

ui_appearance 仓提供三层 API 接口栈：Native C++ Kit、NAPI/JS Kit、ANI/ArkTS Kit。三层接口均委托给 UiAppearanceAbilityClient（IPC 客户端代理），最终通过 IPC 调用 UiAppearanceAbility SA 服务。

本文档只提供稳定的源码、SDK 声明、测试和 Spec 路由。具体行为、默认值、边界条件和兼容性说明以对应 SDK 声明、源码实现、测试用例和 Spec 为准。

## 快速路由

### 源码入口

| 关注点 | 稳定路径 | 说明 |
|--------|----------|------|
| Native Kit 实现 | `interfaces/kits/native/src/ui_appearance.cpp` | 函数指针委托到 UiAppearanceAbilityClient |
| Native Kit 头文件 | `interfaces/kits/native/include/ui_appearance.h` | `UIAppearance` 静态类：SetDarkMode/GetDarkMode/SetSettingData |
| 类型定义 | `interfaces/kits/native/include/ui_appearance_types.h` | `DarkMode` 枚举、`UiAppearanceAbilityErrCode` 错误码 |
| NAPI 模块 | `interfaces/kits/napi/src/js_ui_appearance.cpp` | `@ohos.uiAppearance` 注册、async work |
| NAPI 头文件 | `interfaces/kits/napi/include/js_ui_appearance.h` | 异步上下文、JsUiAppearance 类 |
| ANI 模块 | `interfaces/ets/ani/src/ui_appearance.cpp` | `@ohos.uiAppearance.uiAppearance` ANI 绑定 |
| ANI 头文件 | `interfaces/ets/ani/src/ui_appearance.h` | ANI 函数声明 |
| ArkTS 声明 | `interfaces/ets/ani/ets/@ohos.uiAppearance.ets` | ArkTS 类型声明 |

### API 入口

| 层级 | 稳定路径 | 说明 |
|------|----------|------|
| Native C++ | `interfaces/kits/native/include/ui_appearance.h` | `UIAppearance` 静态类：SetDarkMode/GetDarkMode/SetSettingData |
| NAPI/JS | `interfaces/kits/napi/src/js_ui_appearance.cpp` | `@ohos.uiAppearance`：setDarkMode/getDarkMode/setFontScale/getFontScale/setFontWeightScale/getFontWeightScale |
| ANI/ArkTS | `interfaces/ets/ani/ets/@ohos.uiAppearance.ets` | `@ohos.uiAppearance.uiAppearance`：同 NAPI 接口，支持 Promise/Callback |

### 三层接口对照

| 功能 | Native C++ | NAPI/JS | ANI/ArkTS |
|------|-----------|---------|-----------|
| 设置深色模式 | `SetDarkMode(DarkMode)` | `setDarkMode(mode)` | `setDarkMode(mode)` |
| 获取深色模式 | `GetDarkMode(DarkMode&)` | `getDarkMode()` | `getDarkMode()` |
| 设置字体缩放 | — | `setFontScale(fontScale)` | `setFontScale(fontScale)` |
| 获取字体缩放 | — | `getFontScale()` | `getFontScale()` |
| 设置字体粗细缩放 | — | `setFontWeightScale(fontWeightScale)` | `setFontWeightScale(fontWeightScale)` |
| 获取字体粗细缩放 | — | `getFontWeightScale()` | `getFontWeightScale()` |
| 设置通用数据 | `SetSettingData(string, string)` | — | — |

### 错误码

| 错误码 | 值 | 说明 |
|--------|-----|------|
| `SUCCEEDED` | 0 | 成功 |
| `PERMISSION_ERR` | 201 | 缺少权限 |
| `NOT_SYSTEM_APP` | 202 | 非系统应用 |
| `INVALID_ARG` | 401 | 参数无效 |
| `SYS_ERR` | 500001 | 系统错误 |

### 测试入口

| 稳定路径 | 用途 |
|----------|------|
| `test/unittest/ui_appearance_test.cpp` | API 测试：SetDarkMode/GetDarkMode、SetFontScale/GetFontScale、SetFontWeightScale/GetFontWeightScale、权限检查 |

### 相关主题

| 主题 | 路由 |
|------|------|
| UI外观服务 | `docs/kb/service/ui-appearance-service.md` |
| 架构设计 | `docs/kb/architecture/ui-appearance-architecture.md` |

## 常见问题定位

| 问题 | 优先查看 |
|------|----------|
| SetDarkMode 返回 201 | 权限 `ohos.permission.UPDATE_CONFIGURATION` |
| SetSettingData 返回 202 | 系统应用检查：`TokenIdKit::IsSystemAppByFullTokenID` |
| Native Kit 不支持 SetFontScale/SetFontWeightScale | 这两个接口仅通过 NAPI/ANI 暴露 |
| NAPI 模块加载失败 | `libuiappearance.so` 安装路径 |
| ANI Promise/Callback 模式选择 | `interfaces/ets/ani/src/ui_appearance.cpp`：传入 Callback 用 Callback 模式，否则返回 Promise |

## 调试入口

- 日志标签：`UiAppearance`
- NAPI 异步工作：`napi_async_work` 的 Execute/Complete 阶段
- Native Kit 函数指针：`UIAppearance::setDarkModeFunc_` / `getDarkModeFunc_` / `setSettingDataFunc_`
