# 安装说明

## 方案一：安装到引擎目录

1. 将本仓库复制到：
   `Engine/Plugins/Developer/BlueprintTextExporter`
2. 如有需要，重新生成工程文件。
3. 编译编辑器目标。
4. 启动 Unreal Editor。
5. 如果插件未自动启用，请在 Plugins 窗口中启用 `Blueprint Text Exporter`。

## 推荐构建命令

```powershell
RunUAT.bat BuildPlugin `
  -Plugin="D:\Path\To\BlueprintTextExporter.uplugin" `
  -Package="D:\Path\To\BlueprintTextExporterBuild"
```

## 模块信息

- 模块名：`BlueprintTextExporter`
- 模块类型：`Editor`

## 适用说明

- Unreal Engine 5.7 版本使用 UE5 风格接口
- Unreal Engine 4.26 版本需要使用已适配的 UE4.26 兼容实现
