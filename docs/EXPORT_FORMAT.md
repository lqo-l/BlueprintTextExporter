# 导出格式

## Blueprint

Blueprint 导出会生成两种文件：

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

Material 导出同样会生成两种文件：

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
- `Notes`
- `DeclaredParameters`
- `Graph`

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
- `notes`
- `declaredParameters`
- `roots`

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
