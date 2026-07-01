# BlueprintTextExporter

一个面向 Unreal Editor 的导出插件，用于把 `Blueprint` 和 `Material` 图逻辑导出为：

- 便于人类阅读的 `.txt`
- 便于程序处理的 `.json`

它的目标不是还原 Shader 编译细节，而是把图逻辑转换成适合调试、评审、文档沉淀和 AI 理解的文本资产。

## 核心能力

- 在 Content Browser 中右键导出 Blueprint 执行流
- 在 Content Browser 中右键导出 Material、Material Instance、Material Function 图结构
- 同时输出 `.txt` 与 `.json`
- 导出 Blueprint 纯节点输入信息，减少只看执行线时的上下文缺失
- 导出材质根属性链路，如 `Emissive Color`、`Base Color` 等
- 导出材质声明过的参数，包括“暴露了但未接入最终输出”的参数
- 导出 Material Function 依赖与输出链
- 导出完成后可直接从通知中打开目标输出目录

## 适用场景

- 团队内部做 Blueprint / Material 文档化
- 不打开编辑器截图，也能快速理解图逻辑
- 给代码评审、TA 评审或技术美术分析提供文本上下文
- 作为 AI 助手、图可视化工具、对比工具的输入
- 做资源回归排查，观察导出结果差异

## 当前支持

- Blueprint 执行流导出
- Blueprint 纯节点输入展开
- Material 根属性追踪
- Material Instance 参数说明导出
- Material / Function 暴露参数清单导出
- 未接入最终输出的声明参数保留
- Material Function 输出链追踪
- Static Switch 图导出

## 编辑器菜单

- `Export Blueprint Text + JSON`
- `Export Material Text + JSON`

## 输出位置

- `Saved/BlueprintExports/...`
- `Saved/MaterialExports/...`

## 快速示例

导出后的 Blueprint 文本通常类似：

```text
Event BeginPlay
  then ->
    Print String
```

导出后的 Material 文本通常包含：

```text
Asset: /Game/...
AssetKind: Material
MaterialDomain: MD_LightFunction
DeclaredParameters:
  Scalar Intensity = 1
Graph:
  [Root] Emissive Color
    Clamp
      Input <-
        Switch Param (False) / 'EnablePerShadow'
```

## 兼容性说明

当前仓库面向以下通用引擎版本整理：

- Unreal Engine 5.7
- Unreal Engine 4.26

更详细的兼容说明见：[docs/COMPATIBILITY.md](docs/COMPATIBILITY.md)

## 文档导航

- [安装说明](docs/INSTALL.md)
- [验证说明](docs/VALIDATION.md)
- [导出格式说明](docs/EXPORT_FORMAT.md)
- [兼容性说明](docs/COMPATIBILITY.md)

## 仓库结构

- `BlueprintTextExporter.uplugin`：插件清单
- `Config/`：插件过滤与打包配置
- `Source/`：插件源码
- `docs/`：安装、验证、格式和兼容性文档

## 已验证内容

已通过以下流程验证插件可打包构建：

```powershell
RunUAT.bat BuildPlugin `
  -Plugin="D:\Path\To\BlueprintTextExporter.uplugin" `
  -Package="D:\Path\To\BlueprintTextExporterBuild"
```

已验证能力包括：

- Blueprint 文本导出
- 材质图导出
- 材质暴露参数导出
- 导出通知中打开输出目录
- UE5.7 / UE4.26 兼容构建

## 后续可继续增强

- 紧凑 JSON 模式
- 面向大模型的摘要模式
- 重复子图折叠
- 节点语义增强
- 可选去除冗余 `id` 字段

## 说明

- 这是一个仅编辑器可用的插件
- 这个仓库更适合做源码展示、内部工具沉淀或二次开发
- 如果要公开发布，建议补充最终仓库地址、License、示例截图和发布说明
