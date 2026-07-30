# Smart Gesture Manager Context

> 文档版本：v1.0
> 更新时间：2026-07-30
> 来源：`docs/context_registry.json` 主题 `SmartGestureManager`

## 定位

SmartGestureManager 是 ui_appearance 仓中负责智慧手势模式管理的单例管理器。它支持两种模式：禁用和自动，通过 SettingDataManager 监听 `persist.gesture.smart_gesture_enable` 设置数据变化，并在模式变化时通知上层回调。

本文档只提供稳定的源码、API、测试和 Spec 路由。具体行为、默认值、边界条件和兼容性说明以对应源码实现、测试用例和 Spec 为准。

## 快速路由

### 源码入口

| 关注点 | 稳定路径 | 说明 |
|--------|----------|------|
| 管理器实现 | `services/src/smart_gesture_manager.cpp` | `SmartGestureManager`：Initialize、RegisterSettingDataObserver、模式切换回调 |
| 管理器头文件 | `services/include/smart_gesture_manager.h` | `SmartGestureMode` 枚举、公开/私有接口声明 |

### 模式定义

| 模式 | 枚举值 | DataShare Key | 说明 |
|------|--------|---------------|------|
| `SMART_GESTURE_DISABLE` | 0 | `persist.gesture.smart_gesture_enable` | 禁用智慧手势 |
| `SMART_GESTURE_AUTO` | 1 | `persist.gesture.smart_gesture_enable` | 自动模式（开启智慧手势） |

### 测试入口

| 稳定路径 | 用途 |
|----------|------|
| `test/unittest/smart_gesture_manager_test/smart_gesture_manager_test.cpp` | 智慧手势测试：LoadSettingDataObserversCallback、RegisterSettingDataObserver 成功/失败 |

### 相关主题

| 主题 | 路由 |
|------|------|
| UI外观服务 | `docs/kb/service/ui-appearance-service.md` |
| 架构设计 | `docs/kb/architecture/ui-appearance-architecture.md` |

## 常见问题定位

| 问题 | 优先查看 |
|------|----------|
| 智慧手势模式不生效 | `persist.gesture.smart_gesture_enable` 值是否为 0 或 1；`LoadSettingDataObserversCallback` |
| 观察者注册失败 | `SettingDataManager::RegisterObserver` 返回值、DataShare 可用性 |
| 模式切换后 Configuration 未更新 | `updateCallback` 是否绑定到 `UiAppearanceAbility::UpdateSmartGestureModeCallback` |

## 调试入口

- 日志标签：`UiAppearance`
- 关键方法：`LoadSettingDataObserversCallback`、`OnChangeSmartGestureMode`
- DataShare：`settings get persist.gesture.smart_gesture_enable`
