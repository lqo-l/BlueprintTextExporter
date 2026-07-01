# Installation

[English](INSTALL.md) | [简体中文](INSTALL.zh-CN.md)

## Location

Recommended plugin path:

`Engine/Plugins/Developer/BlueprintTextExporter`

You can also place it under your project `Plugins/` directory if that better fits your workflow.

## Steps

1. Copy this repository to the target plugin directory
2. Regenerate project files if needed
3. Build the editor target
4. Launch Unreal Editor
5. Confirm that `Blueprint Text Exporter` is enabled in the Plugins window

## Build Command

```powershell
RunUAT.bat BuildPlugin `
  -Plugin="D:\Path\To\BlueprintTextExporter.uplugin" `
  -Package="D:\Path\To\BlueprintTextExporterBuild"
```

Expected result:

- `BUILD SUCCESSFUL`

## Module

- Module: `BlueprintTextExporter`
- Type: `Editor`

## Tested Versions

- Unreal Engine 5.7
- Unreal Engine 4.26
