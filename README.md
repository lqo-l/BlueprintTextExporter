# BlueprintTextExporter

![UE 5.7](https://img.shields.io/badge/UE-5.7-0E1128)
![UE 4.26](https://img.shields.io/badge/UE-4.26-0E1128)
![Blueprint](https://img.shields.io/badge/Asset-Blueprint-1F6FEB)
![Material](https://img.shields.io/badge/Asset-Material-238636)
![TXT + JSON](https://img.shields.io/badge/Output-TXT%20%2B%20JSON-8250DF)

Export Blueprint and Material graphs from Unreal Engine as readable `.txt` and structured `.json`.

这个插件面向 Unreal Editor，目标是把原本只能在图编辑器里查看的逻辑，转换成适合阅读、评审、归档和 AI 理解的文本结果。

## Features

- Right-click export for `Blueprint`
- Right-click export for `Material` / `MaterialInstance` / `MaterialFunction`
- Human-readable `.txt` output for quick inspection
- Structured `.json` output for tools and pipelines
- Inline pure-input context for Blueprint graphs
- Root-property graph traversal for Material graphs
- Declared parameter export, including exposed-but-unused parameters
- One-click open output folder after export

## Supported Assets

- Blueprint
- Material
- Material Instance
- Material Function

## Quick Start

1. Copy this plugin to `Engine/Plugins/Developer/BlueprintTextExporter` or your Unreal plugin directory.
2. Build the editor target.
3. Enable `Blueprint Text Exporter` in the Plugins window if needed.
4. Right-click an asset in the Content Browser and run:
   - `Export Blueprint Text + JSON`
   - `Export Material Text + JSON`

Detailed setup steps: [docs/INSTALL.md](docs/INSTALL.md)

## Output

Exported files are written to:

- `Saved/BlueprintExports/...`
- `Saved/MaterialExports/...`

Blueprint text output is designed for fast reading:

```text
Event BeginPlay
  then ->
    Print String
```

Material text output includes metadata, parameters and graph roots:

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

Format details: [docs/EXPORT_FORMAT.md](docs/EXPORT_FORMAT.md)

## Why TXT and JSON

- Use `.txt` when the goal is quick reading, reviews, or feeding concise context into AI
- Use `.json` when the goal is automation, structured parsing, or diff-friendly processing

## Compatibility

Tested with:

- Unreal Engine 5.7
- Unreal Engine 4.26

Compatibility notes: [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md)

## Validation

Recommended validation flow:

1. Run `BuildPlugin`
2. Verify the context-menu entries in the editor
3. Export one Blueprint and one Material asset
4. Check both `.txt` and `.json` outputs

Validation checklist: [docs/VALIDATION.md](docs/VALIDATION.md)

## Design Notes

- The goal is graph understanding, not shader reconstruction
- Material export keeps declared parameters even when they are not connected to final outputs
- JSON favors completeness over compactness
- Some repeated subgraphs may appear more than once in exported output when that improves traceability

## Documentation

- [Installation](docs/INSTALL.md)
- [Validation](docs/VALIDATION.md)
- [Export Format](docs/EXPORT_FORMAT.md)
- [Compatibility](docs/COMPATIBILITY.md)
