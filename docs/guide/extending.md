# Extending (Custom Types)

For a more complete "typed flag" experience, you can use `clasp::Value`:

- Implement `Value::type()` / `string()` / `set(value)`
- Bind: `bindFlagValue("--flag", value)` or use `withValueFlag(...)`

Values are applied after parsing + env/config merge (set in occurrence order).
