# UiAppearance Architecture Context

> 文档版本：v1.0
> 更新时间：2026-07-30
> 来源：`docs/context_registry.json` 主题 `UiAppearanceArchitecture`

## 定位

ui_appearance 仓采用 SA（System Ability）服务架构，通过 IDL 定义的 IPC 接口实现客户端-服务端通信。核心架构包括：SA 注册与生命周期管理、IPC Stub/Proxy 生成、三层 API 接口栈、多用户与子配置支持、DataShare 数据持久化、定时器调度。

本文档只提供稳定的源码、API、测试和 Spec 路由。具体行为、默认值、边界条件和兼容性说明以对应源码实现、测试用例和 Spec 为准。

## 快速路由

### 源码入口

| 关注点 | 稳定路径 | 说明 |
|--------|----------|------|
| IDL 接口定义 | `services/IUiAppearanceAbility.idl` | 7 个 IPC 方法定义，生成 Stub + Proxy |
| IPC 客户端 | `services/src/ui_appearance_ability_client.cpp` | `UiAppearanceAbilityClient` 单例、死亡通知、重连机制 |
| IPC 客户端头文件 | `services/include/ui_appearance_ability_client.h` | `UiAppearanceDeathRecipient`、`GetInstance` |
| 用户上下文 | `services/src/account_context.cpp` | `AccountContext` 结构、`AccountContextHelper` 静态方法 |
| 用户上下文头文件 | `services/include/account_context.h` | `userId` + `subProfileId`、比较运算符 |
| 数据持久化 | `services/utils/src/setting_data_manager.cpp` | `SettingDataManager` 单例：DataShare CRUD、观察者注册 |
| 数据持久化头文件 | `services/utils/include/setting_data_manager.h` | `GetStringValue`/`SetStringValue`/`RegisterObserver` 等 |
| 数据观察者 | `services/utils/src/setting_data_observer.cpp` | `SettingDataObserver`：DataAbility OnChange 分发 |
| 数据观察者头文件 | `services/utils/include/setting_data_observer.h` | `UpdateFunc` 类型、`CreateObserver` |
| 系统参数封装 | `services/utils/src/parameter_wrap.cpp` | `GetParameter`/`SetParameter` 封装 |
| JSON 工具 | `services/utils/src/json_utils.cpp` | JSON 文件加载（白名单等） |

### 架构概览

```
┌─────────────────────────────────────────────────────────┐
│  应用层                                                  │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐    │
│  │ Native C++   │ │ NAPI/JS      │ │ ANI/ArkTS    │    │
│  │ UIAppearance │ │ @ohos.ui...  │ │ @ohos.ui...  │    │
│  └──────┬───────┘ └──────┬───────┘ └──────┬───────┘    │
│         └────────────────┼────────────────┘            │
│                          ▼                              │
│  ┌───────────────────────────────────────────────┐     │
│  │ UiAppearanceAbilityClient (IPC 客户端代理)     │     │
│  │  · 单例 · SAMGR 获取 SA · 死亡通知重连         │     │
│  └───────────────────┬───────────────────────────┘     │
│                      │ IPC                              │
│  ┌───────────────────▼───────────────────────────┐     │
│  │ UiAppearanceAbility (SA 7002, 服务端)           │     │
│  │  · UiAppearanceAbilityStub (IDL 生成)           │     │
│  │  · 多用户 UiAppearanceParam                     │     │
│  │  · DarkModeManager / SmartGestureManager        │     │
│  │  · BackGroundAppColorSwitchSettings             │     │
│  └───────────────────────────────────────────────┘     │
│         │              │              │                  │
│    DataShare      AlarmTimer      SystemParam           │
│    (设置数据)     (定时调度)      (持久化)              │
└─────────────────────────────────────────────────────────┘
```

### SA 配置

| 项 | 值 |
|----|-----|
| SA ID | 7002 |
| 进程 | `ui_service` |
| 库 | `libui_appearance_service.z.so` |
| 启动方式 | run-on-create |

### 构建目标

| 目标 | 产物 | 说明 |
|------|------|------|
| `ui_appearance_service` | `libui_appearance_service.z.so` | SA 服务端 |
| `ui_appearance_client` | `libui_appearance_client.z.so` | IPC 客户端（platformsdk innerapi） |
| `ui_appearance_ability_stub` | source_set | IDL 生成 Stub |
| `ui_appearance_kit` | `libui_appearance_kit.z.so` | Native C++ Kit（platformsdk） |
| `uiappearance` | `libuiappearance.so` | NAPI JS 模块 |
| `ui_appearance_ani` | `libui_appearance_ani.so` | ANI ArkTS 模块 |
| `ui_appearance_abc` | `ui_appearance.abc` | ArkTS 字节码 |

### 权限模型

| 方法 | 权限 | 错误码 |
|------|------|--------|
| SetDarkMode | `ohos.permission.UPDATE_CONFIGURATION` | 201 |
| SetFontScale | `ohos.permission.UPDATE_CONFIGURATION` | 201 |
| SetFontWeightScale | `ohos.permission.UPDATE_CONFIGURATION` | 201 |
| SetSettingData | 系统应用（`TokenIdKit::IsSystemAppByFullTokenID`） | 202 |
| Get* | 无 | — |

### 多用户与子配置

- `AccountContext`：`{ userId, subProfileId }`，`subProfileId = -1` 表示无子配置
- 参数键按用户后缀区分：`persist.ace.darkmode.<userId>`、`persist.sys.font_scale_for_user.<userId>`
- 子配置（车机 `ENABLE_MULTIPLE_OS_ACCOUNT_SUBSPACE`）：参数键附加 `.subProfileId`

### 系统参数默认值

| 参数 | 默认值 | 来源 |
|------|--------|------|
| `persist.ace.darkmode` | `light` | `etc/para/ui_appearance.para` |
| `persist.sys.font_scale_for_user0` | `1.0` | `etc/para/ui_appearance.para` |
| `persist.sys.font_wght_scale_for_user0` | `1.0` | `etc/para/ui_appearance.para` |
| `persist.uiAppearance.first_initialization` | `1` | `etc/para/ui_appearance.para` |

### 测试入口

| 稳定路径 | 用途 |
|----------|------|
| `test/unittest/ui_appearance_test.cpp` | SA 主类测试 |
| `test/unittest/setting_data_manager_test/setting_data_manager_test.cpp` | SettingDataManager CRUD、观察者测试 |
| `test/unittest/setting_data_observer_test/setting_data_observer_test.cpp` | Observer 回调测试 |
| `test/mock/` | Mock 基础设施：mock_parameter、mock_accesstoken_kit、mock_app_mgr_proxy 等 |

### 相关主题

| 主题 | 路由 |
|------|------|
| UI外观服务 | `docs/kb/service/ui-appearance-service.md` |
| 深色模式调度 | `docs/kb/feature/dark-mode-manager.md` |
| API 接口 | `docs/kb/api/ui-appearance-api.md` |

## 常见问题定位

| 问题 | 优先查看 |
|------|----------|
| SA 服务未启动 | `ui_service` 进程、SA 7002 注册状态 |
| IPC 调用失败 | `UiAppearanceDeathRecipient::OnRemoteDied`、proxy 重连 |
| 多用户设置不生效 | `AccountContext` 构建、参数键后缀、`UserSwitchFunc` |
| DataShare 读写失败 | `SettingDataManager::CreateDataShareHelper`、URI 拼接 |
| IDL 生成文件冲突 | 不手动编辑 `${target_gen_dir}/`，修改 `.idl` 后重新构建 |
| 兼容性迁移问题 | `DoCompatibleProcess`、`persist.uiAppearance.first_initialization` |

## 调试入口

- 日志标签：`UiAppearance`，日志域：`0xD003900`
- SA 状态：`system_ability_status 7002`
- 系统参数：`param get persist.ace.darkmode`
- DataShare：`settings list`
- 客户端重连：`UiAppearanceAbilityClient::GetUiAppearanceServiceProxy`
