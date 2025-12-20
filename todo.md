# TODO (Cobra-Like Feature Checklist)

Goal: implement a C++17 CLI framework that mirrors Go cobra's behavior and ergonomics.

## Plan (Next: Full Cobra Parity)

- [ ] 定义“对齐范围”：明确必须支持/可选支持/不支持的 Cobra 能力（以 Cobra `Command` API + CLI 行为为准）
- [ ] 功能差距盘点：逐项对照 Cobra（help/usage 模板、completion 回调、flag 类型/语义、命令分组/排序等）
- [ ] 分阶段落地：每个缺口补齐对应示例 + CTest（先核心 CLI 行为，再生态与可选项）
- [ ] 兼容性与稳定性：完善错误信息/输出一致性、边界用例（unknown flags/args、组合 flags、`--` 等）
- [ ] 文档收尾：README + 文档生成/完成脚本用法更新

## P0 Core (Make It Usable)

- [x] Command tree
- [x] `Use/Short/Long` metadata
- [x] Subcommand registration + lookup
- [x] `Run/RunE` (return exit code / error)
- [x] Args validation helpers (NoArgs / ExactArgs / MinimumNArgs / MaximumNArgs / RangeArgs)
- [x] Basic flag parsing
- [x] `--long` / `-s`
- [x] `--k=v`
- [x] `--` end-of-flags
- [x] bool/string/int/float values + defaults
- [x] Help & usage
- [x] `--help/-h`
- [x] `help [cmd]` subcommand
- [x] Usage template: Usage / Commands / Flags
- [x] Version
- [x] `--version` and/or `version` subcommand

## P1 Usability (Closer To Cobra)

- [x] Hooks
- [x] `PreRun/PreRunE/PostRun/PostRunE`
- [x] `PersistentPreRun/...` (inherit)
- [x] Persistent flags (inherit from parents)
- [x] Local vs persistent flag display
- [x] Required/hidden/deprecated
- [x] Required flags
- [x] Hidden flags/commands
- [x] Deprecated messages
- [x] Error/usage behavior
- [x] `SilenceUsage`
- [x] `SilenceErrors`
- [x] `SetOut/SetErr` (stdout/stderr routing)
- [x] Suggestions
- [x] Unknown command suggestions (closest match)
- [x] Flag suggestions (closest match)
- [x] Flag parsing details (optional)
- [x] `DisableFlagParsing`
- [x] `TraverseChildren` (allow child flags before command)
- [x] Flag groups: `MarkFlagsMutuallyExclusive/OneRequired/RequiredTogether`
- [x] Command aliases
- [x] Short flag grouping (e.g. `-abc`)
- [x] Bool negation (optional, e.g. `--no-foo`)

## P2 Optional Ecosystem

- [x] Shell completion
- [x] bash/zsh/fish/powershell completion generation
- [x] Docs generation
- [x] markdown/manpage generation
- [x] Config integration (viper-like, optional)
- [x] env/config file binding + precedence merge
