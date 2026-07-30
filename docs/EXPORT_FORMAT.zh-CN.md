# 导出格式

[English](EXPORT_FORMAT.md) | [简体中文](EXPORT_FORMAT.zh-CN.md)

## Blueprint

Blueprint 导出会生成：

- `.txt`：用于阅读执行流
- `.json`：用于结构化处理

### TXT

TXT 重点强调：

- 执行顺序
- 节点层级
- 纯节点输入上下文

### JSON

JSON 重点强调：

- 节点标识
- 输入连接关系
- 可供工具消费的结构化图数据

## Material

Material 导出同样会生成：

- `.txt`：图树 + 元数据
- `.json`：结构化图数据 + 元数据

## Material TXT 区块

常见区块包括：

- `Asset`
- `AssetKind`
- `Parent`
- `MaterialDomain`
- `BlendMode`
- `TwoSided`
- `DependentFunctions`
- `DependentFunctionExports`
- `Notes`
- `DeclaredParameters`
- `Graph`
- `CustomHLSL`

## Material JSON 字段

常见字段包括：

- `assetPath`
- `assetName`
- `assetKind`
- `materialDomain`
- `blendMode`
- `twoSided`
- `parentPath`
- `dependentFunctions`
- `dependentFunctionExports`
- `notes`
- `declaredParameters`
- `roots`
- `customHlsl`

实际可达的 Material Function 会递归导出为各自独立的 TXT 和 JSON。调用方图中保留 Function Call 节点，`dependentFunctionExports` 将每个函数资产路径映射到对应导出文件。`customHlsl` 只属于当前材质或函数，并完整保留 Custom 节点源码及配置。

## Declared Parameters

`declaredParameters` 表示材质、材质函数或材质接口上声明过的参数。

即使参数没有连接到最终根输出，它们仍然会被保留，便于：

- 审核参数设计
- 补全图逻辑上下文
- 做实例或版本之间的对比

单个参数项通常包含：

- `name`
- `type`
- `value`
- `group`
- `sortPriority`
- `description`
- `sourceAssetPath`

## 取舍

- TXT 更适合直接阅读
- JSON 更适合工具链处理
- JSON 会比 TXT 更冗长
- 导出结果优先保证可追踪性和信息完整度
