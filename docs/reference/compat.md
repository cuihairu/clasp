# 兼容范围（Cobra v1.x）

本项目对 “Cobra parity” 的定义是：给定同一棵命令树 + argv，Clasp 尽量匹配 Cobra v1.x 的**可观测 CLI 行为**（而不是复制 Go API surface）。

## Must Support（兼容契约）

这些属于“兼容性承诺”：调整行为应补齐对应 example + CTest。

- 命令树：子命令、aliases、hidden/deprecated、groups、suggestions
- hooks：persistent + local（继承与顺序）
- args validators：NoArgs/Exact/Min/Max/Range
- flags：persistent vs local、required/hidden/deprecated、group constraints、重复 flags
- 解析语义：`--k=v`、`-k v`、`-k=v`、`-abc`、`-ovalue`、`--no-foo`、`--`
- help/usage/version：`--help/-h`、`help [cmd]`、default values 展示、排序开关、examples
- completion：bash/zsh/fish/powershell + `__complete` candidates + directives
- 外部源合并：`flag > env > config > default`

## Should Support（Best-Effort）

这些对 “Cobra-like feel” 很重要，但暂时不作为严格 contract（除非 CTest 锁定）：

- Cobra 默认英文文案/空白的逐字节一致
- 更“怪”的 pflag edge cases（排序、错误文案细节、罕见交互）
- completion 脚本在 shell/runtime 中体现的细节差异（Clasp 只输出 directives）

## Non-Goals（明确不追）

- Go template 语言等价（Clasp 仅做占位符替换 + 完全覆盖函数）
- 复制 Cobra/pflag 的全部 Go API surface（`*pflag.FlagSet` 等）
- 完整 pflag 类型面（可通过 `clasp::Value` 扩展）
- 完整 Viper drop-in

## 已知差异（当前可接受）

- 模板语言能力受限（无条件/循环/管道/func）
- `--help/-h` 与 `--version` 作为内建 flags（即使未显式声明）
- 内建类型面是子集（`bool/int/int64/uint64/float/double/duration/string` + 若干 helpers）
- completion 细节依赖 shell/runtime
- 错误文案不保证逐字节一致（除非用 CTest 锁定）

## 把 best-effort 变成 contract 的推荐流程

1) 在 `examples/` 增加复现/示例
2) 在 `CMakeLists.txt` 增加 CTest 断言（锁定输出/exit code/关键语义）
3) 实现行为并保证跨平台一致（macOS/Linux/Windows）
