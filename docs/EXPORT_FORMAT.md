# 导出格式说明

## Blueprint 导出

Blueprint 导出会生成：

- `.txt`：可读的执行树文本
- `.json`：结构化节点图数据

适用场景包括：

- 调试
- 代码评审或脚本评审
- 文档整理
- AI 辅助解释

## Material 导出

Material 导出会生成：

- `.txt`：可读图树 + 元数据
- `.json`：结构化图数据 + 元数据

## Material TXT 主要区块

- `Asset`
- `AssetKind`
- `Parent`（若存在）
- `MaterialDomain`
- `BlendMode`
- `TwoSided`（当值为 true 时）
- `DependentFunctions`
- `Notes`
- `DeclaredParameters`
- `Graph`

## Material JSON 主要字段

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

## Declared Parameters 说明

`declaredParameters` 用于表示该材质、材质函数或材质接口上“声明过/暴露过”的参数。

即使这些参数没有接入最终根输出，它们仍然应该被保留下来，方便：

- 审核材质参数设计
- 给大模型提供完整参数上下文
- 比较不同材质实例或函数暴露出来的控制项

当前每个参数项包含：

- `name`
- `type`
- `value`
- `group`
- `sortPriority`
- `description`
- `sourceAssetPath`

## 当前取舍

- 当前导出更偏向完整性和可追踪性
- 重复子图可能会重复展开
- JSON 对工具处理更友好，但对大模型来说可能偏冗长
- TXT 通常更适合直接阅读和第一轮 AI 理解

## 后续可继续增强的方向

- 紧凑 JSON 模式
- 语义摘要模式
- 重复子树折叠
- 面向大模型的可选 `id` 去除模式
