# 兼容性

## 目标版本

当前支持以下引擎版本：

- Unreal Engine 5.7
- Unreal Engine 4.26

## Unreal Engine 5.7

支持：

- Blueprint 导出
- Material 导出
- Declared Parameters 导出
- 导出完成后打开输出目录

## Unreal Engine 4.26

为适配 UE4.26，已处理的典型差异包括：

- `FAppStyle` / `FEditorStyle`
- `FAssetData::GetClass()` 的旧版接口差异
- `FNotificationInfo` 字段差异
- 部分 Material 导出 API 差异
- `StaticSwitch` / `StaticSwitchParameter` 导出差异

## 使用建议

在目标引擎版本接入后，建议至少完成以下检查：

1. `BuildPlugin`
2. 编辑器内菜单是否正常出现
3. Blueprint 导出是否正常
4. Material 导出是否正常
5. `.txt` 与 `.json` 是否同时生成

## 可能需要额外适配的情况

- 引擎分支修改了材质节点 API
- 参数系统或元数据接口发生变化
- Content Browser 扩展接口有差异
- 工程里存在自定义材质节点，需要额外导出规则
