# TODO (Cobra-Like Feature Checklist)

Goal: implement a C++17 CLI framework that mirrors Go cobra's behavior and ergonomics.

## Status (2025-12-21)

- Feature checklist below (P0–P2) is implemented and covered by CTest (142 tests passing in `CMakeLists.txt`).
- The “Cobra parity” items tracked below (P3) are implemented; remaining work is defining scope/compat guarantees and polishing edge-case fidelity.

## Plan (Next: Full Cobra Parity)

- [x] 定义“对齐范围”：明确必须支持/可选支持/不支持的 Cobra 能力（以 Cobra `Command` API + CLI 行为为准），见 `COMPAT.md`
- [ ] 功能差距盘点：逐项对照 Cobra（help/usage/version 模板、completion 回调/指令、flag 类型/语义、命令分组/排序、错误/usage 输出一致性等）
- [ ] 分阶段落地：每个缺口补齐对应示例 + CTest（先核心 CLI 行为，再生态与可选项）
- [ ] 兼容性与稳定性：完善错误信息/输出一致性、边界用例（unknown flags/args、组合 flags、`--` 等）
- [ ] 文档收尾：README + 文档生成/完成脚本用法更新

## Next Session TODO (Suggested)

- [ ] 明确“Full Cobra parity”目标：哪些 Cobra/pflag API 必须做，哪些永远不做（补齐到 `COMPAT.md`）
- [ ] 继续补 pflag 类型（只做实际需要的）：IP/net、更多 map/slice 变体
- [ ] 补齐边界用例一致性：`--`、unknown flags/args、组合 flags、错误文案对齐
- [ ] 文档收尾：README/EXAMPLES 更新（新增 bytes/external typed 等能力）

## Cobra Parity Additions (P3)

> These were originally tracked as “likely missing” relative to Cobra/pflag; they are now implemented and covered by examples/CTest.

### P3 Parity (API Surface)

- [x] `Example` / `Examples` strings (show in help + docs)
- [x] `Annotations` (free-form key/value metadata on `Command` / flags)
- [x] Command groups (`AddGroup`, group ID/title, grouped help output)
- [x] Custom help/usage/version templates + funcs (Cobra: `SetHelpTemplate/SetUsageTemplate/SetVersionTemplate/...`)
- [x] Config integration beyond `.env`-style key/value (optional: YAML/JSON/TOML + nesting)
- [x] Programmatic execution helpers (Cobra: `SetArgs`, `Execute` variants); C++ equivalent: `setArgs(vector<string>)`, `execute()`

### P3 Parity (Flags / pflag Semantics)

- [x] More flag types: `int64/uint/double/duration`, plus slice/map helpers (`getFlagValues*`, `getFlagMap`) (pflag parity is large; define target subset)
- [x] Repeated flags semantics (`--tag a --tag b`) and slice parsing rules
- [x] Per-flag “no option default value” (pflag `NoOptDefVal`, optional values)
- [x] Flag normalization (underscore/dash normalization hook)
- [x] Expose parser knobs currently hard-coded in `Parser::Options`:
  - [x] allow unknown flags (Cobra: `FParseErrWhitelist.UnknownFlags`)
  - [x] toggle short-flag grouping
  - [x] toggle `--no-foo` bool negation

### P3 Parity (Completion)

- [x] Completion directives / richer semantics (Cobra: “no file completion”, file ext filters, keep order, etc.)
- [x] File/dir flag helpers + `--flag=value` behavior (Cobra: `MarkFlagFilename/MarkFlagDirname`, completion scripts handle `=` form)
- [x] Completion options to disable default commands (`completion`, `__complete*`) or change their naming

### P3 Parity (Help Output Fidelity)

- [x] Show default values in help (Cobra’s `--flag` default formatting)
- [x] Stable sorting options (Cobra sorts commands/flags by default; add opt-in/out to match)

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

## P4 Future / Nice-to-have (Gaps vs Cobra/pflag)

> Not required for the “Cobra-like” scope in `COMPAT.md`, but useful if you want closer parity.

- [x] `DisableFlagsInUseLine` (usage line formatting toggle)
- [x] `SetFlagErrorFunc`-style override for parse/validation errors
- [x] `SetHelpCommand`-style customization (rename/disable/override help command)
- [ ] More pflag type parity (beyond current “useful subset”)
  - [x] Slice getters: `getStringSlice/getBoolSlice/getIntSlice/getInt64Slice/getUint64Slice/getFloatSlice/getDoubleSlice/getDurationSlice`
  - [x] Array getters: `getStringArray/getBoolArray/getIntArray/getInt64Array/getUint64Array/getFloatArray/getDoubleArray/getDurationArray`
  - [x] Map getters: `getStringToString/getStringToInt/getStringToInt64/getStringToUint64/getStringToBool`
  - [x] `count` flags + `getCount`
  - [x] Custom `Value` interface + env/config merge
  - [x] Strict type parsing (match pflag errors; reject trailing junk like `1,2` for numeric flags)
  - [ ] Additional pflag types (IP/net, more slice/map variants) if needed
  - [x] Byte size flag (human-friendly `bytes`, e.g. `1KB`, `1.5MiB`)
- [x] More completion parity (dynamic scripts + directive mapping + file-ext/dir filters)
- [x] More config formats (YAML ✅ / TOML ✅) and richer nested binding rules (lists/arrays ✅)
- [x] Context propagation (`ExecuteContext`-style)
