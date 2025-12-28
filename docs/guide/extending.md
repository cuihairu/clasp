# Extending（自定义类型）

如果你想要更完整的“typed flag”体验，可以用 `clasp::Value`：

- 实现 `Value::type()` / `string()` / `set(value)`
- 绑定：`bindFlagValue("--flag", value)` 或使用 `withValueFlag(...)`

Value 会在解析 + env/config merge 之后应用（按 occurrence 顺序 set）。
