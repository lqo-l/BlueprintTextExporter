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
- `Notes`
- `DeclaredParameters`
- `Graph`

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
- `notes`
- `declaredParameters`
- `roots`

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
