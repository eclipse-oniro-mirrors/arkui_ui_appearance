# UI外观架构知识库

> 更新时间：2026-07-30
> 源码路径：`services/`

## 概述

ui_appearance 仓采用 SA（System Ability）服务架构，通过 IDL 定义的 IPC 接口实现客户端-服务端通信。核心架构包括：SA 注册与生命周期管理、IPC Stub/Proxy 生成、三层 API 接口栈、多用户与子配置支持、DataShare 数据持久化、定时器调度。

## 目录结构

```text
ui_appearance/
├── sa_profile/
│   └── 7002.json              # SA 配置（SA ID=7002, process=ui_service）
├── etc/para/
│   ├── ui_appearance.para     # 系统参数默认值
│   └── ui_appearance.para.dac # 系统参数 DAC 权限
├── services/
│   ├── IUiAppearanceAbility.idl # IDL 接口定义
│   ├── src/                     # 服务实现
│   └── utils/                   # 工具类
├── interfaces/
│   ├── kits/native/             # C++ Native Kit
│   ├── kits/napi/               # NAPI/JS Kit
│   └── ets/ani/                 # ANI/ArkTS Kit
└── test/                        # 单元测试 + Mock
```

## 核心类继承关系

```
SystemAbility + UiAppearanceAbilityStub
  └── UiAppearanceAbility (SA 7002, 服务端)

IUiAppearanceAbility (IDL 接口)
  ├── UiAppearanceAbilityStub (IDL 生成, 服务端)
  └── UiAppearanceAbilityProxy (IDL 生成, 客户端)

RefBase
  └── UiAppearanceAbilityClient (单例, 客户端代理)
        └── UiAppearanceDeathRecipient (死亡通知)

UIAppearance (静态类, Native Kit)
  └── 委托到 UiAppearanceAbilityClient

JsUiAppearance (NAPI Kit)
  └── 委托到 UiAppearanceAbilityClient

ANI 绑定函数 (ANI Kit)
  └── 委托到 UiAppearanceAbilityClient
```

## 实现详解

### SA 注册与生命周期

**SA 配置**：`sa_profile/7002.json`

```json
{
    "name": 7002,
    "libpath": "/system/lib/libui_appearance_service.z.so",
    "run-on-create": true,
    "distributed": false
}
```

**生命周期流程**：

```
SA Framework 加载 → OnStart()
  → PublishSA(7002)
  → 监听 APP_MGR_SERVICE_ID
  → OnAddSystemAbility(APP_MGR_SERVICE_ID)
    → DoCompatibleProcess()         // 首次升级兼容
    → DarkModeManager::Initialize() // 深色模式初始化
    → SmartGestureManager::Initialize() // 智慧手势初始化
    → SubscribeCommonEvent()        // 订阅公共事件
    → DoInitProcess()              // 用户初始化
```

### IPC 架构

**IDL 定义**：`services/IUiAppearanceAbility.idl`

```idl
interface OHOS.ArkUi.UiAppearance.IUiAppearanceAbility {
    int SetDarkMode([in] int darkMode);
    int GetDarkMode();
    int GetFontScale([out] String fontScale);
    int SetFontScale([in] String fontScale);
    int GetFontWeightScale([out] String fontWeightScale);
    int SetFontWeightScale([in] String fontWeightScale);
    int SetSettingData([in] String key, [in] String value);
}
```

**IDL 代码生成**：
- `UiAppearanceAbilityStub` — 服务端 Stub，处理 IPC 请求分发
- `UiAppearanceAbilityProxy` — 客户端 Proxy，封装 IPC 调用
- 生成路径：`${target_gen_dir}/`（构建时由 IDL 工具生成）

**客户端代理**：
- `UiAppearanceAbilityClient` — 单例，通过 SAMGR 获取 SA 代理
- `UiAppearanceDeathRecipient` — 监听 SA 死亡，触发重连

### 三层 API 接口栈

| 层级 | 文件路径 | 构建目标 | 用途 |
|---|---|---|---|
| **Native C++ Kit** | `interfaces/kits/native/` | `libui_appearance_kit.z.so` | C++ 静态类，platformsdk innerapi |
| **NAPI/JS Kit** | `interfaces/kits/napi/` | `libuiappearance.so` | `@ohos.uiAppearance` JS 模块 |
| **ANI/ArkTS Kit** | `interfaces/ets/ani/` | `libui_appearance_ani.so` + `ui_appearance.abc` | ArkTS Native Interface |

所有三层均委托到 `UiAppearanceAbilityClient`，最终通过 IPC 调用 SA 服务。

### 多用户与子配置

**AccountContext**：

```cpp
struct AccountContext {
    int32_t userId;
    int32_t subProfileId;  // INVALID_SUB_PROFILE_ID(-1) 表示无子配置
};
```

**多用户支持**：
- 每个用户维护独立的 `UiAppearanceParam`（darkMode, fontScale, fontWeightScale）
- 通过 `AccountContext` 索引用户参数
- 参数键按用户后缀区分：`persist.ace.darkmode.<userId>`、`persist.sys.font_scale_for_user.<userId>`

**子配置支持（车机平台）**：
- 编译宏 `ENABLE_MULTIPLE_OS_ACCOUNT_SUBSPACE` 启用
- `AccountContext` 包含 `subProfileId` 字段
- 参数键和设置键附加 `.subProfileId` 后缀
- 子配置切换时将中心用户外观应用到目标子配置

### DataShare 数据持久化

**SettingDataManager**（单例）封装 DataShare 操作：

| 操作 | 方法 | 说明 |
|---|---|---|
| 读取字符串 | `GetStringValue(key, value, userId)` | 从 settings 数据库读取 |
| 读取整数 | `GetInt32Value(key, value, userId)` | 从 settings 数据库读取 |
| 读取布尔值 | `GetBoolValue(key, value, userId)` | 从 settings 数据库读取 |
| 写入字符串 | `SetStringValue(key, value, userId, needNotify)` | 写入 settings 数据库 |
| 写入整数 | `SetInt32Value(key, value, userId, needNotify)` | 写入 settings 数据库 |
| 写入布尔值 | `SetBoolValue(key, value, userId, needNotify)` | 写入 settings 数据库 |
| 注册观察者 | `RegisterObserver(key, updateFunc, userId)` | 监听数据变化 |
| 反注册观察者 | `UnregisterObserver(key, userId)` | 取消监听 |

**URI 模式**：
- 全局数据库：`datashare:///com.ohos.settingsdata/entry/settings/data/<KEY>`
- 用户数据库：`datashare:///com.ohos.settingsdata/entry/settings/data/<USER_ID>/<KEY>`

### 系统参数

| 参数 | 默认值 | 用途 |
|---|---|---|
| `persist.ace.darkmode` | `light` | 深色模式（用户 0，旧版） |
| `persist.ace.darkmode.<userId>` | — | 每用户深色模式 |
| `persist.sys.font_scale_for_user0` | `1.0` | 用户 0 字体缩放 |
| `persist.sys.font_scale_for_user.<userId>` | — | 每用户字体缩放 |
| `persist.sys.font_wght_scale_for_user0` | `1.0` | 用户 0 字体粗细缩放 |
| `persist.sys.font_wght_scale_for_user.<userId>` | — | 每用户字体粗细缩放 |
| `persist.uiAppearance.first_initialization` | `1` | 首次启动标志 |
| `persist.uiAppearance.dark_mode_temp_state_flag.<ctx>` | — | 临时颜色模式标志 |
| `const.standard_font_weight` | — | 系统默认字体粗细 |

### 权限模型

| 方法 | 权限 | 错误码 |
|---|---|---|
| `SetDarkMode` | `ohos.permission.UPDATE_CONFIGURATION` | 201 |
| `SetFontScale` | `ohos.permission.UPDATE_CONFIGURATION` | 201 |
| `SetFontWeightScale` | `ohos.permission.UPDATE_CONFIGURATION` | 201 |
| `SetSettingData` | 系统应用（`TokenIdKit::IsSystemAppByFullTokenID`） | 202 |
| `GetDarkMode` | 无 | — |
| `GetFontScale` | 无 | — |
| `GetFontWeightScale` | 无 | — |

### 构建目标

| 目标 | 类型 | 产物 |
|---|---|---|
| `ui_appearance_service` | shared_library | `libui_appearance_service.z.so` |
| `ui_appearance_client` | shared_library | `libui_appearance_client.z.so` |
| `ui_appearance_ability_stub` | source_set | IDL 生成的 Stub 代码 |
| `ui_appearance_kit` | shared_library | `libui_appearance_kit.z.so` |
| `uiappearance` | shared_library | `libuiappearance.so` |
| `ui_appearance_ani` | shared_library | `libui_appearance_ani.so` |
| `ui_appearance_abc` | generate_static_abc | `ui_appearance.abc` |

## 完整 API 清单

### IDL 接口

```cpp
int SetDarkMode([in] int darkMode);
int GetDarkMode();
int GetFontScale([out] String fontScale);
int SetFontScale([in] String fontScale);
int GetFontWeightScale([out] String fontWeightScale);
int SetFontWeightScale([in] String fontWeightScale);
int SetSettingData([in] String key, [in] String value);
```

## 关键实现细节

### Configuration 更新流程

```
SetDarkMode(mode) → VerifyAccessToken → OnSetDarkMode
  → UpdateCurrentUserConfiguration → UpdateConfiguration
    → AppMgr::UpdateConfiguration (通知所有应用进程)
    → ConfigurePersistence (持久化到系统参数)
```

### 客户端重连机制

```
UiAppearanceAbilityClient::GetUiAppearanceServiceProxy()
  → 如果 proxy 为空 → CreateUiAppearanceServiceProxy()
    → SAMGR::GetSystemAbility(7002)
    → 注册 UiAppearanceDeathRecipient

UiAppearanceDeathRecipient::OnRemoteDied()
  → 清空 proxy → 下次调用时自动重连
```

### IDL 生成流程

```
IUiAppearanceAbility.idl
  → idl_gen_interface("ui_appearance_ability_interface")
    → ${target_gen_dir}/iui_appearance_ability_stub.cpp
    → ${target_gen_dir}/iui_appearance_ability_proxy.cpp
    → ${target_gen_dir}/iui_appearance_ability.h
```

### 兼容性迁移

`DoCompatibleProcess()` 在首次升级时执行：
1. 检查 `persist.uiAppearance.first_initialization` 参数
2. 如果为首次启动，从旧版参数迁移到新版参数格式
3. 设置 `persist.uiAppearance.first_initialization=0` 标记迁移完成

## 使用示例

### 完整的设置深色模式调用链

```
JS App: uiAppearance.setDarkMode(DarkMode.ALWAYS_DARK)
  → NAPI async work: UiAppearanceAbilityClient::SetDarkMode(ALWAYS_DARK)
    → IPC: UiAppearanceAbilityProxy::SetDarkMode(0)
      → SA Stub: UiAppearanceAbility::SetDarkMode(0)
        → VerifyAccessToken("ohos.permission.UPDATE_CONFIGURATION")
        → OnSetDarkMode(context, ALWAYS_DARK)
          → DarkModeManager::NotifyDarkModeUpdate
          → UpdateConfiguration (AppMgr)
          → ConfigurePersistence (系统参数)
```

## 调试指南

- 日志标签：`UiAppearance`
- 日志域：`0xD003900`
- 系统参数查看：`param get persist.ace.darkmode`
- DataShare 数据查看：`settings list`
- SA 状态查看：`system_ability_status 7002`

## 常见问题

1. **SA 服务未启动**：检查 `ui_service` 进程是否运行，SA 7002 是否已注册
2. **IPC 调用失败**：检查 SA 进程是否崩溃，`UiAppearanceDeathRecipient` 是否触发重连
3. **多用户设置不生效**：确认 `AccountContext` 的 userId 是否正确，参数键是否包含正确的用户后缀
4. **DataShare 读写失败**：检查 DataShare 服务是否可用，URI 是否正确
5. **IDL 生成文件冲突**：不要手动编辑 `${target_gen_dir}/` 下的生成文件，修改 `IUiAppearanceAbility.idl` 后重新构建
