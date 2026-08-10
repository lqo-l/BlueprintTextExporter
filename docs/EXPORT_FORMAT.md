# Export Format

[English](EXPORT_FORMAT.md) | [简体中文](EXPORT_FORMAT.zh-CN.md)

## Blueprint

Blueprint export produces:

- `.txt` for readable execution flow
- `.json` for structured graph data

### TXT

TXT focuses on:

- Execution order
- Node hierarchy
- Pure-input context

### JSON

JSON focuses on:

- Node identifiers
- Input connections
- Tool-friendly structured graph data

## Material

Material export also produces:

- `.txt` for graph trees and metadata
- `.json` for structured graph data and metadata

## Material TXT Sections

Common sections include:

- `Asset`
- `AssetKind`
- `Parent`
- `MaterialDomain`
- `BlendMode`
- `TwoSided`
- `DependentFunctions`
- `DependentFunctionExports`
- `Notes`
- `DeclaredParameters`
- `Graph`
- `UnusedGraph`
- `Diagnostics`
- `CustomHLSL`

## Material JSON Fields

Common fields include:

- `assetPath`
- `assetName`
- `assetKind`
- `materialDomain`
- `blendMode`
- `twoSided`
- `parentPath`
- `dependentFunctions`
- `dependentFunctionExports`
- `notes`
- `declaredParameters`
- `roots`
- `unusedNodes`
- `compileDiagnostics`
- `nodeDiagnostics`
- `customHlsl`

Reachable Material Functions are exported recursively to their own TXT and JSON files. Function-call nodes remain in the caller graph, while `dependentFunctionExports` maps each function asset path to its output files. `customHlsl` belongs only to the current material or function and preserves complete Custom-node source and configuration.

`Graph`/`roots` contains the graph that contributes to a material root or Material Function output. `UnusedGraph`/`unusedNodes` preserves disconnected authoring components, including editor positions. Unused Custom nodes are also included in `customHlsl`, but unused Material Function calls do not trigger recursive dependency export.

`Diagnostics` preserves errors available in the current Editor session. `compileDiagnostics` contains errors cached by the active shader-platform material resource, while `nodeDiagnostics` maps cached compile errors and expression `LastErrorText` to node IDs. Empty diagnostics do not prove that every platform compiles successfully; compile the material for the platform of interest before export when authoritative diagnostics are required.

## Declared Parameters

`declaredParameters` represents parameters declared on a material, material function, or material interface.

These parameters are preserved even when they are not connected to the final root output. That makes them useful for:

- Parameter audits
- Graph-context completion
- Comparing instances or revisions

Each parameter item typically includes:

- `name`
- `type`
- `value`
- `group`
- `sortPriority`
- `description`
- `sourceAssetPath`

## Tradeoffs

- TXT is easier to read directly
- JSON is easier for downstream tooling
- JSON is usually more verbose
- Export favors traceability and completeness
