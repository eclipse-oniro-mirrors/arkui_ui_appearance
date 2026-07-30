# UiAppearance Knowledge Base

此目录用于存放 ui_appearance 仓的知识库文档，并通过索引文件支持快速检索。

> 更新时间：2026-07-30
> 适用仓库：`OpenHarmony/foundation/arkui/ui_appearance/docs/kb`

## 1. 检索入口

- 上下文索引：`docs/knowledge_base_INDEX.json`
- 兜底检索：`rg -n "<关键字>" docs/kb`

## 2. 当前统计

- `docs/kb/` 下知识库文档总数：5
- `docs/knowledge_base_INDEX.json` 索引条目总数：5

类型分布：
- `service`: 1
- `feature`: 2
- `api`: 1
- `architecture`: 1

分类分布：
- `service`: 1
- `feature`: 2
- `api`: 1
- `architecture`: 1

## 3. 目录结构

```text
docs/
├── knowledge_base_INDEX.json
└── kb/
    ├── README.md
    ├── service/
    │   └── UiAppearance_Service_Knowledge_Base_CN.md
    ├── feature/
    │   ├── DarkMode_Manager_Knowledge_Base_CN.md
    │   └── SmartGesture_Manager_Knowledge_Base_CN.md
    ├── api/
    │   └── UiAppearance_API_Knowledge_Base_CN.md
    └── architecture/
        └── UiAppearance_Architecture_Knowledge_Base_CN.md
```

## 4. 索引维护规则

新增或更新知识库时，至少同步以下内容：

1. 更新文档本体。
2. 更新 `docs/knowledge_base_INDEX.json`：
   - 必填：`name/name_cn/category/type/keywords/aliases/file_path/last_updated`
   - 推荐：`source_paths/api_paths`
   - 分类取值：`service/feature/api/architecture`
3. 更新本文件统计与目录快照。

校验命令：

```bash
python3 -m json.tool docs/knowledge_base_INDEX.json > /dev/null && echo "Valid JSON"
find docs -name "*_Knowledge_Base*.md" -type f | wc -l
```
