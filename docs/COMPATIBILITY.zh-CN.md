# 兼容性

[English](COMPATIBILITY.md) | [简体中文](COMPATIBILITY.zh-CN.md)

## 已验证版本

- Unreal Engine 5.7
- Unreal Engine 4.26

## Unreal Engine 5.7

已验证支持：

- Blueprint 导出
- Material 导出
- Declared Parameters 导出
- 导出完成后打开输出目录

## Unreal Engine 4.26

UE4.26 的典型兼容处理包括：

- `FAppStyle` / `FEditorStyle`
- 旧版 `FAssetData::GetClass()` 接口
- `FNotificationInfo` 字段差异
- Material 导出相关 API 差异
- `StaticSwitch` / `StaticSwitchParameter` 导出差异

## 推荐检查项

在目标引擎版本接入后，建议至少完成：

1. `BuildPlugin`
2. 编辑器内菜单是否正常出现
3. Blueprint 导出是否正常
4. Material 导出是否正常
5. `.txt` 与 `.json` 是否同时生成

## 可能需要额外适配的情况

- 引擎特定的材质节点 API 变更
- 参数系统或元数据接口变化
- Content Browser 扩展接口差异
- 工程内自定义材质节点需要额外导出规则
