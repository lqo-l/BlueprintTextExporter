# Validation

[English](VALIDATION.md) | [简体中文](VALIDATION.zh-CN.md)

## 1. Build

Run:

```powershell
RunUAT.bat BuildPlugin `
  -Plugin="D:\Path\To\BlueprintTextExporter.uplugin" `
  -Package="D:\Path\To\BlueprintTextExporterBuild"
```

Expected result:

- `BUILD SUCCESSFUL`

## 2. Editor

### Blueprint

1. Open the editor
2. Right-click a Blueprint in the Content Browser
3. Run `Export Blueprint Text + JSON`
4. Check `Saved/BlueprintExports/...`

### Material

1. Right-click a `Material`, `MaterialInstance`, or `MaterialFunction`
2. Run `Export Material Text + JSON`
3. Check `Saved/MaterialExports/...`

## 3. What To Check

### Blueprint

- `.txt` is generated
- `.json` is generated
- Execution order is readable
- Pure-input context is visible

### Material

- `MaterialDomain` is present
- At least one root output is visible, such as `Emissive Color` or `Base Color`
- Parameters, texture references, or function calls appear in the graph
- `DeclaredParameters` appears in both `.txt` and `.json`
- Declared-but-unused parameters are still preserved

## 4. Suggested Test Assets

Choose a material that includes:

- Texture sampling
- Multiple parameters
- `Static Switch` or `Static Switch Parameter`
- A clear root-output chain
