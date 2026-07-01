# 兼容性说明

## 当前验证状态

本仓库当前面向两类通用引擎版本整理：

- Unreal Engine 5.7
- Unreal Engine 4.26

## Unreal Engine 5.7

验证点包括：

- Blueprint 导出
- Material / MaterialInstance / MaterialFunction 导出
- 暴露参数导出
- 未接入最终输出的参数保留
- 导出通知中打开输出目录

## Unreal Engine 4.26

为适配 UE4.26，已处理以下典型兼容差异：

- `FAppStyle` / `FEditorStyle` 差异
- `FAssetData::GetClass()` 旧版接口差异
- `FNotificationInfo` 字段差异
- 材质导出中部分 UE5 专用 API 的替换
- `StaticSwitch` / `StaticSwitchParameter` 导出兼容

## 使用建议

如果你要将本仓库用于目标引擎版本，建议优先做一次：

1. `BuildPlugin`
2. 编辑器内右键导出验证
3. Blueprint 与 Material 样例资源回归检查

## 可能需要继续适配的情况

- 材质节点 API 在不同引擎版本中发生变化
- 参数系统或元数据接口发生变化
- 内容浏览器菜单扩展接口差异
- 自定义材质节点需要额外导出规则
