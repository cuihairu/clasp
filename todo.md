# TODO (Cobra-Like Feature Checklist)

Goal: implement a C++17 CLI framework that mirrors Go cobra's behavior and ergonomics.

## Status (2025-12-25)

- Feature checklist below (P0–P3) is implemented and covered by CTest (162 tests passing in `CMakeLists.txt`).
- Compatibility scope is defined in `COMPAT.md`; CI runs CMake/CTest on Ubuntu/macOS/Windows.

## Open Items (2025-12-26)

- Remaining unchecked items: **3**
- Scope/contract: tighten “Should Support” → “Must Support” items (add examples + CTest per item)
- Types: add more pflag type parity only if needed (more slice/map variants and other common types)

## Plan (Next: Full Cobra Parity)

- [x] 定义“对齐范围”：明确必须支持/可选支持/不支持的 Cobra 能力（以 Cobra `Command` API + CLI 行为为准），见 `COMPAT.md`
- [x] 功能差距盘点：逐项对照 Cobra（help/usage/version 模板、completion 回调/指令、flag 类型/语义、命令分组/排序、错误/usage 输出一致性等），见 `COMPAT.md` 的 “Feature Inventory”
- [x] 分阶段落地：每个缺口补齐对应示例 + CTest（先核心 CLI 行为，再生态与可选项）
- [x] 兼容性与稳定性：完善错误信息/输出一致性、边界用例（unknown flags/args、组合 flags、`--` 等）
- [x] 文档收尾：README + 文档生成/完成脚本用法更新

## Next Session TODO (Suggested, Optional)

- [ ] 如果要继续追 Cobra：把 “Should Support” 的内容逐步收紧为 “Must Support”（每项补齐示例 + CTest）
- [ ] 继续补 pflag 类型（仅按需求）：更多 map/slice 变体、其他常用类型
- [x] 发布准备：版本号策略（SemVer + 版本一致性校验）
- [x] 发布准备：安装/打包说明（CMake `cmake --install` + `find_package`）
- [x] 发布准备：变更记录（`CHANGELOG.md`）

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
- [x] Useful pflag type subset (beyond core built-ins)
  - [x] Slice getters: `getStringSlice/getBoolSlice/getIntSlice/getInt64Slice/getUint64Slice/getFloatSlice/getDoubleSlice/getDurationSlice`
  - [x] Array getters: `getStringArray/getBoolArray/getIntArray/getInt64Array/getUint64Array/getFloatArray/getDoubleArray/getDurationArray`
  - [x] Map getters: `getStringToString/getStringToInt/getStringToInt64/getStringToUint64/getStringToBool`
  - [x] `count` flags + `getCount`
  - [x] Custom `Value` interface + env/config merge
  - [x] Strict type parsing (match pflag errors; reject trailing junk like `1,2` for numeric flags)
  - [x] IP/CIDR helpers (`withIPFlag` / `withCIDRFlag`)
  - [x] URL helper (`withURLFlag`)
  - [x] IPNet/IPMask helpers (`withIPNetFlag` / `withIPMaskFlag`)
  - [ ] Additional pflag types (beyond current subset; more net/url types, more slice/map variants) if needed
  - [x] Byte size flag (human-friendly `bytes`, e.g. `1KB`, `1.5MiB`)
- [x] More completion parity (dynamic scripts + directive mapping + file-ext/dir filters)
- [x] More config formats (YAML ✅ / TOML ✅) and richer nested binding rules (lists/arrays ✅)
- [x] Context propagation (`ExecuteContext`-style)
