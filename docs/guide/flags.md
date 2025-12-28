# Flags（pflag-like 解析）

## 声明与类型

使用 `withFlag(long, short, var, desc, defaultValue)` 声明 flag。内置值类型以 C++ 类型为准：

- `bool/int/int64/uint64/float/double/std::chrono::milliseconds/std::string`
- 额外 helpers：`withCountFlag()`、`withBytesFlag()`、`withIPFlag/withCIDRFlag/withIPNetFlag/withIPMaskFlag/withURLFlag()`

## 常见解析语义

- `--k=v`、`-k=v`、`-k v`
- `-abc`（short grouping，可配置）
- `--no-foo`（bool negation，可配置）
- `--` 结束 flag 解析
- 重复 flag：多次出现时保留 occurrence 顺序

## NoOptDefVal（可选值）

通过 `markFlagNoOptDefaultValue("--mode", "auto")`，允许 `--mode` 在不带参数时取默认值。

## pflag 风格的 slice/map 取值

Clasp 不强制“typed slice/map flags”，但提供解析辅助（基于重复值/逗号分隔）：

- `Parser::getStringSlice/getIntSlice/...`
- `Parser::getStringToString/getStringToInt/...`
