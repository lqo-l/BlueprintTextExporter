# 验证说明

## 构建验证

执行：

```powershell
RunUAT.bat BuildPlugin `
  -Plugin="D:\Path\To\BlueprintTextExporter.uplugin" `
  -Package="D:\Path\To\BlueprintTextExporterBuild"
```

期望结果：

- `BUILD SUCCESSFUL`

## 编辑器验证

### Blueprint 导出验证

1. 打开编辑器。
2. 在 Content Browser 中右键一个 Blueprint 资源。
3. 点击 `Export Blueprint Text + JSON`。
4. 确认文件输出到：
   `Saved/BlueprintExports/...`

### Material 导出验证

1. 在 Content Browser 中右键一个 `Material`、`MaterialInstance` 或 `MaterialFunction`。
2. 点击 `Export Material Text + JSON`。
3. 确认文件输出到：
   `Saved/MaterialExports/...`

## 材质导出重点检查项

- 存在 `MaterialDomain`
- 当根属性已连接时，可以看到如 `Emissive Color` 这类根输出
- 图结构中可以看到纹理引用和参数名
- `.txt` 与 `.json` 中都存在 `DeclaredParameters`
- 即使某些参数没有接入最终输出，它们也仍然会出现在 `DeclaredParameters`

## 示例验证资源

建议选择一个同时包含以下特征的材质资源进行验证：

- 基于世界坐标的滚动噪声采样
- 包含 `Static Switch` 或 `Static Switch Parameter`
- 包含多个可调参数与纹理采样
- 能看到参数与根输出链路同时被导出
