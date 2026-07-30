# AGENTS.md

This file defines working rules for agents in `ui_appearance`.

## 1. Scope and Priority

- This file applies to `OpenHarmony/foundation/arkui/ui_appearance`.
- Direct user instructions take priority over this file.
- Principle: **code first, evidence first, no fabrication**.

## 2. Quick Build and Test

### Build

```bash
# Build ui_appearance packages (from OpenHarmony root)
./build.sh --product-name rk3568 --build-target ui_appearance_packages --ccache

# Build individual targets
./build.sh --product-name rk3568 --build-target ui_appearance_service --ccache
./build.sh --product-name rk3568 --build-target ui_appearance_client --ccache
./build.sh --product-name rk3568 --build-target ui_appearance_kit --ccache
./build.sh --product-name rk3568 --build-target uiappearance --ccache
./build.sh --product-name rk3568 --build-target ui_appearance_ani --ccache
```

### Required Validation Policy

- After modifying business code under `services/`, `interfaces/`, or related production paths, the final build evidence must include a complete `ui_appearance_packages` build:

  ```bash
  ./build.sh --product-name rk3568 --build-target ui_appearance_packages --ccache
  ```

- A single object build or a local GN target may be used for fast diagnosis, but does not replace the final complete build.

### Unit Test Build and Run

```bash
# Build all unit tests (from OpenHarmony root)
./build.sh --product-name rk3568 --build-target unittest --ccache

# Build individual test target
./build.sh --product-name rk3568 --build-target ui_appearance_test --ccache
./build.sh --product-name rk3568 --build-target dark_mode_manager_test --ccache
./build.sh --product-name rk3568 --build-target setting_data_manager_test --ccache
./build.sh --product-name rk3568 --build-target setting_data_observer_test --ccache
./build.sh --product-name rk3568 --build-target smart_gesture_manager_test --ccache

# Run test (on device)
hdc shell am instrument -w unittest/ui_appearance_test
```

### Build Outputs Summary

Main output dir: `out/rk3568/arkui/ui_appearance/`

- Service library: `libui_appearance_service.z.so` — SA server (SA ID 7002, process `ui_service`)
- Client library: `libui_appearance_client.z.so` — IPC client proxy (platformsdk innerapi)
- Native kit: `libui_appearance_kit.z.so` — C++ native kit (platformsdk)
- NAPI module: `libuiappearance.so` — `@ohos.uiAppearance` JS module
- ANI module: `libui_appearance_ani.so` — ArkTS Native Interface module
- ArkTS bytecode: `ui_appearance.abc`

## 3. Knowledge Base

Use the KB as the first-stop context before any deep code analysis, and follow the authoring rules below when adding or updating entries. Always treat the KB as context — **then verify against real source code**.

### 3.1 Lookup First

**MANDATORY: Before any code search or analysis on services, architecture, APIs, or patterns, you MUST run a KB query first. Do NOT skip this step and jump straight to source code.**

- Use `rg` in `docs/kb/` for KB lookup:
  - `rg -n "<keyword>" docs/kb`
- Entry points: `docs/kb/README.md`, `docs/knowledge_base_INDEX.json`, and KB directories under `docs/kb/` (for example: `architecture/`, `service/`, `api/`, `feature/`).

### 3.2 Authoring Standard

- Naming/location: use `XXX_Knowledge_Base.md` or `XXX_Knowledge_Base_CN.md`; place under `docs/kb/<category>/` (choose by topic).
- Index metadata (`docs/knowledge_base_INDEX.json`) must include: `name`, `name_cn`, `category`, `type`, `file_path`, `last_updated`, `keywords` (5-15), `aliases` (2-5); recommend `source_paths` and `api_paths`.
- Allowed categories: `service`, `feature`, `api`, `architecture`.
- Required sections in each KB doc: 概述, 目录结构, 核心类继承关系, 实现详解, 完整API清单, 关键实现细节, 使用示例, 调试指南, 常见问题.

Quick checks:

```bash
find docs -name "*_Knowledge_Base*.md" -type f | wc -l
python3 -m json.tool docs/knowledge_base_INDEX.json > /dev/null && echo "Valid JSON"
```

## 4. Core Working Principles (Must Follow)

### 4.1 Actual Code Only

- Always read real code via search/read tools before concluding behavior.
- Always cite file path and line when giving code-level conclusions.
- If code is missing, explicitly state: **"此代码在 ui_appearance 中未找到"**.
- Do not write hypothetical implementation as fact.

### 4.2 Speculation Management

- Any unverified statement must be labeled **"推测"**.
- Try to verify first; only keep speculation when verification is impossible.

### 4.3 Code-First Verification

- User suggestions may be wrong; verify with source before accepting.
- Resolve disagreements with evidence from implementation.

### 4.4 Error Learning

- If a user correction reveals a doc error, update relevant knowledge base docs.
- Record root cause and prevention in the knowledge base when appropriate.

## 5. Project Map

- `services/`: SA service implementation
  - `src/ui_appearance_ability.cpp`: Main SA class (UiAppearanceAbility, SA ID 7002)
  - `src/ui_appearance_ability_client.cpp`: IPC client proxy (singleton)
  - `src/dark_mode_manager.cpp`: Dark mode scheduling logic (custom-auto, sunrise-sunset)
  - `src/dark_mode_temp_state_manager.cpp`: Temporary color mode state management
  - `src/smart_gesture_manager.cpp`: Smart gesture mode (auto/disabled)
  - `src/account_context.cpp`: Multi-user and sub-profile context
  - `src/screen_switch_operator_manager.cpp`: Screen on/off state tracking
  - `src/background_app_color_switch_settings.cpp`: Background app color switch whitelist
  - `utils/`: Utility classes (SettingDataManager, AlarmTimerManager, ParameterWrap, JsonUtils, IpcSkeletonUtils)
  - `IUiAppearanceAbility.idl`: IDL interface definition (generates Stub + Proxy)
- `interfaces/`: Public API layers
  - `kits/native/`: C++ native kit (`UIAppearance` class, `ui_appearance_types.h`)
  - `kits/napi/`: NAPI JS module (`@ohos.uiAppearance`)
  - `ets/ani/`: ANI ArkTS module (`@ohos.uiAppearance.uiAppearance`)
- `sa_profile/`: SA profile (SA ID 7002, process `ui_service`)
- `etc/para/`: System parameter defaults and DAC permissions
- `test/`: Unit tests with mock infrastructure

## 6. Service Architecture

### SA Registration

- SA ID: 7002 (`ARKUI_UI_APPEARANCE_SERVICE_ID`)
- Process: `ui_service`
- Library: `libui_appearance_service.z.so`
- Lifecycle: `OnStart()` → Publish SA → Listen for AppMgr → `OnAddSystemAbility(APP_MGR_SERVICE_ID)` → Initialize managers → Subscribe common events

### IPC Architecture

- IDL: `IUiAppearanceAbility.idl` → generates Stub (server) + Proxy (client)
- Client: `UiAppearanceAbilityClient` (singleton, death recipient for reconnection)

### Three-Layer Interface Stack

| Layer | File | Purpose |
|---|---|---|
| **Native C++ Kit** | `interfaces/kits/native/` | `UIAppearance` class; delegates to `UiAppearanceAbilityClient` |
| **NAPI/JS Kit** | `interfaces/kits/napi/` | `@ohos.uiAppearance` JS module; async work |
| **ANI/ArkTS Kit** | `interfaces/ets/ani/` | `@ohos.uiAppearance.uiAppearance` ArkTS namespace; Promise/Callback |

### Dark Mode Modes

| Mode | Value | Behavior |
|---|---|---|
| `ALWAYS_LIGHT` | 1 | Always light; clears timers |
| `ALWAYS_DARK` | 0 | Always dark; clears timers |
| `CUSTOM_AUTO` | 2 | Custom time range via `settings.uiappearance.darkmode_starttime/endtime` |
| `SUNRISE_SUNSET` | 3 | Sunrise/sunset via `settings.display.sun_set/sun_rise` |

### Permission Model

- **SetDarkMode/SetFontScale/SetFontWeightScale**: Requires `ohos.permission.UPDATE_CONFIGURATION`
- **SetSettingData**: Requires system app (checked via `TokenIdKit::IsSystemAppByFullTokenID`)
- Error codes: `PERMISSION_ERR(201)`, `NOT_SYSTEM_APP(202)`, `INVALID_ARG(401)`, `SYS_ERR(500001)`

## 7. Testing Guidance

- Test path should mirror source layout.
- 5 test suites: `ui_appearance_test`, `dark_mode_manager_test`, `setting_data_manager_test`, `setting_data_observer_test`, `smart_gesture_manager_test`
- Mock infrastructure under `test/mock/` includes: `mock_parameter.cpp`, `mock_accesstoken_kit.cpp`, `mock_app_mgr_proxy.cpp`, plus mock headers for IPC, SAMGR, DataShare, DataObserver, AlarmTimerManager, SettingDataManager
- Run broader regression tests when the impact is large.
- After business code changes, complete the overall `ui_appearance_packages` build before claiming completion.

## 8. Hard Boundaries (Do not / Ask before)

Do not (without explicit user confirmation):

- Change public API signatures/semantics/error codes under `interfaces/` (including ABI-risk changes).
- Change IDL interface `IUiAppearanceAbility.idl` without updating the generated Stub/Proxy.
- Manually edit IDL-generated files under `${target_gen_dir}/`.
- Add dependencies on other OpenHarmony system modules (including `BUILD.gn` `deps/public_deps/data_deps` dependency entries).
- Run destructive or hard-to-recover commands (for example `rm -rf`, `git reset --hard`).

Ask before:

- Any API/ABI compatibility-impacting change or default behavior change.
- Any new/updated/replaced dependency: `bundle.json` dependency changes; new `deps/public_deps/data_deps` in any `BUILD.gn`.
- Changes to SA profile (`sa_profile/7002.json`) or system parameter defaults (`etc/para/`).
- Changes to permission model or error code definitions.
