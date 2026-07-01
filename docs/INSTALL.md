# 安装

## 目录位置

推荐安装到：

`Engine/Plugins/Developer/BlueprintTextExporter`

如果你的工作流更适合按工程隔离，也可以放到工程的 `Plugins/` 目录下。

## 安装步骤

1. 复制本仓库到目标插件目录
2. 重新生成工程文件（如有需要）
3. 编译编辑器目标
4. 启动 Unreal Editor
5. 在 `Plugins` 窗口中确认 `Blueprint Text Exporter` 已启用

## 构建命令

```powershell
RunUAT.bat BuildPlugin `
  -Plugin="D:\Path\To\BlueprintTextExporter.uplugin" `
  -Package="D:\Path\To\BlueprintTextExporterBuild"
```

预期结果：

- `BUILD SUCCESSFUL`

## 模块信息

- Module: `BlueprintTextExporter`
- Type: `Editor`

## 适用版本

- Unreal Engine 5.7
- Unreal Engine 4.26
