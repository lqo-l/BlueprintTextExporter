# BlueprintTextExporter

[English](README.md) | [简体中文](README.zh-CN.md)

![UE 5.7](https://img.shields.io/badge/UE-5.7-0E1128)
![UE 4.26](https://img.shields.io/badge/UE-4.26-0E1128)
![Blueprint](https://img.shields.io/badge/Asset-Blueprint-1F6FEB)
![Material](https://img.shields.io/badge/Asset-Material-238636)
![TXT + JSON](https://img.shields.io/badge/Output-TXT%20%2B%20JSON-8250DF)

将 Unreal Engine 中的 Blueprint 和 Material 图导出为可读的 `.txt` 与结构化的 `.json`。

BlueprintTextExporter 是一个 Unreal Editor 插件，用于把原本只能在图编辑器中查看的逻辑，转换成更适合阅读、评审、归档，以及接入外部工具或 AI 工作流的文本结果。

## 功能特性

- 支持右键导出 `Blueprint`
- 支持右键导出 `Material`、`MaterialInstance`、`MaterialFunction`
- 输出便于阅读的 `.txt`
- 输出便于处理的 `.json`
- Blueprint 导出包含纯节点输入上下文
- Material 导出包含根属性图追踪
- 支持导出声明参数，包括暴露但未接入最终输出的参数
- 导出完成后可一键打开输出目录

## 支持的资源类型

- Blueprint
- Material
- Material Instance
- Material Function

## 快速开始

1. 将插件复制到 `Engine/Plugins/Developer/BlueprintTextExporter` 或你的工程/插件目录。
2. 编译编辑器目标。
3. 如有需要，在 Plugins 窗口中启用 `Blueprint Text Exporter`。
4. 在 Content Browser 中右键资源并执行：
   - `Export Blueprint Text + JSON`
   - `Export Material Text + JSON`

安装说明：[docs/INSTALL.zh-CN.md](docs/INSTALL.zh-CN.md)

## 输出位置

导出文件默认写入：

- `Saved/BlueprintExports/...`
- `Saved/MaterialExports/...`

Blueprint 输出强调执行流可读性：

```text
Event BeginPlay
  then ->
    Print String
```

Material 输出会包含元数据、参数和根属性图：

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

格式说明：[docs/EXPORT_FORMAT.zh-CN.md](docs/EXPORT_FORMAT.zh-CN.md)

## 为什么同时导出 TXT 和 JSON

- `.txt` 适合快速阅读、评审和提供给 AI 的精简上下文
- `.json` 适合自动化、结构化解析和后续工具处理

## 兼容性

已验证：

- Unreal Engine 5.7
- Unreal Engine 4.26

兼容性说明：[docs/COMPATIBILITY.zh-CN.md](docs/COMPATIBILITY.zh-CN.md)

## 验证

推荐验证流程：

1. 运行 `BuildPlugin`
2. 确认编辑器中出现右键导出菜单
3. 分别导出一个 Blueprint 和一个 Material
4. 检查 `.txt` 与 `.json` 是否同时生成

验证清单：[docs/VALIDATION.zh-CN.md](docs/VALIDATION.zh-CN.md)

## 设计说明

- 目标是帮助理解图逻辑，而不是还原 Shader 编译结果
- Material 导出会保留已声明参数，即使这些参数没有接入最终输出
- JSON 更偏向完整性而不是紧凑性
- 为了提高可追踪性，某些重复子图可能会重复展开

## 文档导航

- [安装](docs/INSTALL.zh-CN.md)
- [验证](docs/VALIDATION.zh-CN.md)
- [导出格式](docs/EXPORT_FORMAT.zh-CN.md)
- [兼容性](docs/COMPATIBILITY.zh-CN.md)
- [English Documentation](README.md)
