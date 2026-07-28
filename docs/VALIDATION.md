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

### Current Editor Console

1. Keep the target project Editor open; do not start `UnrealEditor-Cmd.exe`.
2. In the Output Log console, run `BlueprintTextExport.Export /Game/Path/M_Test`.
3. Confirm the Output Log reports the exported `.txt` and `.json` paths.
4. Select a supported asset in the primary Content Browser, run `BlueprintTextExport.Export`, and confirm it exports the selection.
5. Run the command with an invalid path and confirm it reports a non-blocking failure in Output Log without opening a modal dialog.

### MCP Current-Editor Verification (UE 5.7)

Verified in an already running `Client` Editor through `UEEditorMCP`, without starting, closing, or restarting an Editor process:

```python
import unreal; unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(), 'BlueprintTextExport.Export /KuroRender/KuroDynamicEnv/Material/M_BillboardCloudNS,/KuroRender/KuroDynamicEnv/Material/MF/MF_BillboardCloudLighting')
```

Expected evidence:

- `LogBlueprintTextExporterMenu` reports two successful exports.
- Both TXT and JSON files are refreshed under `Saved/MaterialExports`.
- An invalid asset path logs `Cannot load asset` and displays a non-blocking failure notification.

When using an MCP implementation that forwards Python through the Output Log `py` command, keep the code on one line or wrap multi-line code in `exec(...)`; raw indentation-based `for`, `if`, or `try` blocks may otherwise be split into separate console commands.

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
