# UiAppearance KB

> 更新时间：2026-07-30
> 主题数：5
> 适用范围：`OpenHarmony/foundation/arkui/ui_appearance/docs/kb`

`docs/kb/` 是 ui_appearance 仓的知识库专区。新版 KB 只作为 Agent 上下文导航页，行为事实以源码、SDK/API 声明、测试和 Spec 为准。

## 目录约束

```text
docs/kb/
├── service/       # SA 服务核心
├── feature/       # 功能特性模块
├── api/           # API 接口层（Native/NAPI/ANI）
└── architecture/  # 架构设计（IPC、多用户、持久化）
```

## 编写规则

- 新增 KB 必须同步 `docs/context_registry.json`。
- KB 保留定位、源码/API/测试/Spec 路由、调试入口和常见问题定位。
- KB 不长期维护完整 API 行为矩阵、AC/BR/FR/ER/RC 或大段调用链复述。
- 代码级结论必须能从真实源码或 SDK 声明验证，未验证内容标注为"推测"。

## 当前主题

| ID | 主题 | 新版 KB | 状态 |
|----|------|---------|------|
| UiAppearanceService | UI外观服务 | `docs/kb/service/ui-appearance-service.md` | 新建 |
| DarkModeManager | 深色模式管理器 | `docs/kb/feature/dark-mode-manager.md` | 新建 |
| SmartGestureManager | 智慧手势管理器 | `docs/kb/feature/smart-gesture-manager.md` | 新建 |
| UiAppearanceAPI | UI外观API | `docs/kb/api/ui-appearance-api.md` | 新建 |
| UiAppearanceArchitecture | UI外观架构 | `docs/kb/architecture/ui-appearance-architecture.md` | 新建 |

## 检索

```bash
rg -n "<keyword>" docs/kb
python3 -m json.tool docs/context_registry.json > /dev/null
```
