# Templates（Help / Usage / Version）

Clasp 支持两类自定义：

1) **模板字符串**（占位符替换）：`setHelpTemplate/setUsageTemplate/setVersionTemplate`
2) **完全接管**：`setHelpFunc/setUsageFunc`

注意：不追求 Cobra 的 Go template 等价（无循环/条件/管道/func）。
