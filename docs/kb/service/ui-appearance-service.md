# UiAppearance Service Context

> 文档版本：v1.0
> 更新时间：2026-07-30
> 来源：`docs/context_registry.json` 主题 `UiAppearanceService`

## 定位

UiAppearanceAbility 是 ui_appearance 仓的核心 SA（System Ability）服务，SA ID 7002，运行在 `ui_service` 进程中。它负责管理系统的 UI 外观设置，包括深色模式、字体缩放、字体粗细缩放，以及通用设置数据存储。服务通过 IDL 生成的 Stub/Proxy 提供 IPC 接口，支持多用户和子配置（车机平台）。

本文档只提供稳定的源码、API、测试和 Spec 路由。具体行为、默认值、边界条件和兼容性说明以对应源码实现、测试用例和 Spec 为准。

## 快速路由

### 源码入口

| 关注点 | 稳定路径 | 说明 |
|--------|----------|------|
| SA 主类 | `services/src/ui_appearance_ability.cpp` | `UiAppearanceAbility`：OnStart/OnStop 生命周期、IPC 接口实现、Configuration 更新、多用户管理 |
| SA 头文件 | `services/include/ui_appearance_ability.h` | `UiAppearanceParam`、`UiAppearanceEventSubscriber`、核心方法声明 |
| IDL 接口 | `services/IUiAppearanceAbility.idl` | 7 个 IPC 方法：SetDarkMode/GetDarkMode/SetFontScale/GetFontScale/SetFontWeightScale/GetFontWeightScale/SetSettingData |
| SA 配置 | `sa_profile/7002.json` | SA ID=7002, process=ui_service, run-on-create=true |

### API 入口

| 层级 | 稳定路径 | 说明 |
|------|----------|------|
| Native C++ Kit | `interfaces/kits/native/include/ui_appearance.h` | `UIAppearance` 静态类：SetDarkMode/GetDarkMode/SetSettingData |
| 类型定义 | `interfaces/kits/native/include/ui_appearance_types.h` | `DarkMode` 枚举、`UiAppearanceAbilityErrCode` 错误码 |
| NAPI/JS | `interfaces/kits/napi/src/js_ui_appearance.cpp` | `@ohos.uiAppearance` 模块注册 |
| ANI/ArkTS | `interfaces/ets/ani/src/ui_appearance.cpp` | `@ohos.uiAppearance.uiAppearance` ArkTS 绑定 |

### 测试入口

| 稳定路径 | 用途 |
|----------|------|
| `test/unittest/ui_appearance_test.cpp` | SA 主类测试：SetDarkMode/GetDarkMode、SetFontScale/GetFontScale、SetFontWeightScale/GetFontWeightScale、权限检查、AlarmTimer、BackGroundAppColorSwitch |

### 相关主题

| 主题 | 路由 |
|------|------|
| 深色模式调度 | `docs/kb/feature/dark-mode-manager.md` |
| 智慧手势管理 | `docs/kb/feature/smart-gesture-manager.md` |
| 架构设计 | `docs/kb/architecture/ui-appearance-architecture.md` |

## 常见问题定位

| 问题 | 优先查看 |
|------|----------|
| SetDarkMode 返回 201 | 权限检查：`VerifyAccessToken("ohos.permission.UPDATE_CONFIGURATION")` |
| SetSettingData 返回 202 | 系统应用检查：`TokenIdKit::IsSystemAppByFullTokenID` |
| 深色模式切换不生效 | `UpdateConfiguration` → AppMgr::UpdateConfiguration 调用链 |
| 多用户场景外观不生效 | `AccountContext` 构建、`UserSwitchFunc`、`SwitchAppearanceContext` |
| SA 服务未启动 | `ui_service` 进程、SA 7002 注册状态 |
| 后台应用颜色切换失败 | `BackGroundAppColorSwitchSettings`、`/etc/dark_mode_whilelist.json` |

## 调试入口

- 日志标签：`UiAppearance`，日志域：`0xD003900`
- SA 生命周期：`OnStart` / `OnStop` / `OnAddSystemAbility`
- Configuration 更新：`UpdateConfiguration` / `UpdateCurrentUserConfiguration`
- 公共事件：`COMMON_EVENT_USER_SWITCHED` / `COMMON_EVENT_BOOT_COMPLETED` / `COMMON_EVENT_SCREEN_ON`
