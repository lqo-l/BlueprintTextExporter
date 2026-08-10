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

### 已打开 Editor 的控制台

1. 保持目标项目 Editor 已打开；不要启动 `UnrealEditor-Cmd.exe`。
2. 在 Output Log 控制台执行 `BlueprintTextExport.Export /Game/Path/M_Test`。
3. 确认 Output Log 输出导出的 `.txt` 和 `.json` 路径。
4. 在主 Content Browser 选中一个受支持资源，执行 `BlueprintTextExport.Export`，确认导出该选中资源。
5. 传入无效路径执行命令，确认 Output Log 报告非阻塞失败，且不弹出模态对话框。

### MCP 当前 Editor 验证（UE 5.7）

已在正在运行的 `Client` Editor 中通过 `UEEditorMCP` 验证，过程中未启动、关闭或重启 Editor：

```python
import unreal; unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(), 'BlueprintTextExport.Export /KuroRender/KuroDynamicEnv/Material/M_BillboardCloudNS,/KuroRender/KuroDynamicEnv/Material/MF/MF_BillboardCloudLighting')
```

预期证据：

- `LogBlueprintTextExporterMenu` 输出两个资源的导出成功日志。
- `Saved/MaterialExports` 下对应 TXT 和 JSON 的更新时间刷新。
- 无效路径输出 `Cannot load asset`，并显示非阻塞失败通知。

此外已在正在运行的 `Client` Editor 中验证 `/KuroRender/KuroDynamicEnv/Material/Stars/M_Stars`
的 `Use Material Attributes` 导出。刷新后的 TXT 包含 `[Root] Material Attributes`，其下存在非空的
`Switch Param` 与 `SetMaterialAttributes` 节点图。

若 MCP 实现通过 Output Log 的 `py` 命令转发 Python，请保持代码单行，或使用 `exec(...)` 包装多行代码；直接传递包含 `for`、`if`、`try` 等缩进结构的多行脚本可能被拆成多条控制台命令。

## 3. 输出检查项

### Blueprint

- 生成 `.txt`
- 生成 `.json`
- 执行流顺序可读
- 纯节点输入信息可见

### Material

- 存在 `MaterialDomain`
- 能看到至少一个根属性输出，如 `Emissive Color` 或 `Base Color`
- 对启用 `Use Material Attributes` 的材质，存在 `[Root] Material Attributes`，且其子图不为空
- 命名重路由 Usage 包含 `Declaration` 边，并从 Declaration 的 `Input` 继续连接上游图
- 实际可达的函数调用存在 `dependentFunctionExports` 指向的独立 TXT/JSON，调用方不再含 `FunctionOutput:` 子图
- 完整 Custom 源码仅出现在所属材质或函数文件的 `CustomHLSL`/`customHlsl` 中
- 图中能看到参数名、纹理引用或函数调用
- `.txt` 和 `.json` 中都包含 `DeclaredParameters`
- 未接入最终输出的已声明参数仍会被保留
- 未连接节点组件出现在 `UnusedGraph`/`unusedNodes`，并保留编辑器坐标
- 当前缓存的材质错误出现在 `Diagnostics`、`compileDiagnostics` 或 `nodeDiagnostics`；验证该项前应先编译材质

## 4. 建议测试素材

建议选择同时满足以下条件的材质：

- 包含纹理采样
- 包含多个参数
- 包含 `Static Switch` 或 `Static Switch Parameter`
- 根属性上有明确输出链路
