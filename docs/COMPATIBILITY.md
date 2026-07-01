# Compatibility

[English](COMPATIBILITY.md) | [简体中文](COMPATIBILITY.zh-CN.md)

## Tested Versions

- Unreal Engine 5.7
- Unreal Engine 4.26

## Unreal Engine 5.7

Validated support includes:

- Blueprint export
- Material export
- Declared parameter export
- Open-output-folder action after export

## Unreal Engine 4.26

Typical compatibility work for UE4.26 includes:

- `FAppStyle` / `FEditorStyle`
- Older `FAssetData::GetClass()` usage
- `FNotificationInfo` field differences
- Material-export API differences
- `StaticSwitch` / `StaticSwitchParameter` export differences

## Recommended Checks

After integrating into a target engine build, verify at least:

1. `BuildPlugin`
2. Context-menu entries appear in the editor
3. Blueprint export works
4. Material export works
5. Both `.txt` and `.json` are generated

## Cases That May Need Extra Adaptation

- Engine-specific material node API changes
- Parameter-system or metadata API changes
- Content Browser extension differences
- Custom material nodes that require additional export rules
