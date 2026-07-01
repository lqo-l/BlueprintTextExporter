# 验证

[English](VALIDATION.md) | [简体中文](VALIDATION.zh-CN.md)

## 1. 构建验证

执行：

```powershell
RunUAT.bat BuildPlugin `
  -Plugin="D:\Path\To\BlueprintTextExporter.uplugin" `
  -Package="D:\Path\To\BlueprintTextExporterBuild"
```

期望结果：

- `BUILD SUCCESSFUL`

## 2. 编辑器验证

### Blueprint

1. 打开编辑器
2. 在 Content Browser 中右键一个 Blueprint
3. 点击 `Export Blueprint Text + JSON`
4. 检查 `Saved/BlueprintExports/...`

### Material

1. 在 Content Browser 中右键一个 `Material`、`MaterialInstance` 或 `MaterialFunction`
2. 点击 `Export Material Text + JSON`
3. 检查 `Saved/MaterialExports/...`

## 3. 输出检查项

### Blueprint

- 生成 `.txt`
- 生成 `.json`
- 执行流顺序可读
- 纯节点输入信息可见

### Material

- 存在 `MaterialDomain`
- 能看到至少一个根属性输出，如 `Emissive Color` 或 `Base Color`
- 图中能看到参数名、纹理引用或函数调用
- `.txt` 和 `.json` 中都包含 `DeclaredParameters`
- 未接入最终输出的已声明参数仍会被保留

## 4. 建议测试素材

建议选择同时满足以下条件的材质：

- 包含纹理采样
- 包含多个参数
- 包含 `Static Switch` 或 `Static Switch Parameter`
- 根属性上有明确输出链路
