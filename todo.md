# TODO (Cobra-Like Feature Checklist)

Goal: C++17 CLI framework mirroring Go Cobra's behavior and ergonomics.

Status: all planned requirements implemented; 0 open items.
Evidence: commit a2a7823 · 569/569 CTest green · 100% line/function/branch coverage (gcovr, `include/` + `src/`).

## P0 Core

- [x] Command tree
- [x] Use/Short/Long metadata
- [x] Subcommand registration and lookup
- [x] Run/RunE (return exit code or error)
- [x] Args validation (NoArgs/ExactArgs/MinimumNArgs/MaximumNArgs/RangeArgs)
- [x] Basic flag parsing (`--long`, `-s`, `--k=v`, `--` end-of-flags)
- [x] bool/string/int/float values with defaults
- [x] Help and usage generation
- [x] `--help/-h` and `help [cmd]` subcommand
- [x] Usage template (Usage/Commands/Flags)
- [x] Version (`--version` and/or `version` subcommand)

## P1 Usability

- [x] Hooks (PreRun/PreRunE/PostRun/PostRunE)
- [x] PersistentPreRun (inherit)
- [x] Persistent flags (inherit from parents)
- [x] Local vs persistent flag display
- [x] Required/hidden/deprecated flags
- [x] Error/usage behavior (SilenceUsage/SilenceErrors)
- [x] SetOut/SetErr (stdout/stderr routing)
- [x] Suggestions (unknown command and flag, closest match)
- [x] DisableFlagParsing
- [x] TraverseChildren (child flags before command)
- [x] Flag groups (MarkFlagsMutuallyExclusive/OneRequired/RequiredTogether)
- [x] Command aliases
- [x] Short flag grouping (`-abc`)
- [x] Bool negation (`--no-foo`)

## P2 Optional Ecosystem

- [x] Shell completion generation (bash/zsh/fish/powershell)
- [x] Dynamic `__complete`/`__completeNoDesc` + configurable completion command names
- [x] Docs generation (markdown/manpage)
- [x] Config integration (env binding + config file merge + precedence)

## P3 Parity (API Surface)

- [x] Example/Examples strings
- [x] Annotations
- [x] Command groups (AddGroup)
- [x] Custom help/usage/version templates and funcs
- [x] Config formats `.env`/`.ini`/`.cfg`/`.json`/`.toml`/`.yaml` with nesting; unknown extensions rejected
- [x] Programmatic execution (setArgs/execute)

## P3 Parity (Flags / pflag Semantics)

- [x] More flag types (int64/uint/double/duration)
- [x] Narrow widths (int8/int16/int32/uint8/uint16/uint)
- [x] Slice/array/map getters
- [x] ipSlice, bytesBase64, count flags, bytes size flags
- [x] IP/CIDR/IPNet/IPMask/URL helpers
- [x] Repeated flags (`--tag a --tag b`) and slice parsing rules
- [x] NoOptDefVal optional values
- [x] Flag normalization (underscore/dash hook)
- [x] Expose parser knobs (allow unknown flags, short grouping, `--no-foo` negation)
- [x] Custom Value interface + env/config merge
- [x] Strict type parsing (reject trailing junk like `1,2`)

## P3 Parity (Completion)

- [x] Completion directives (no-file-completion, file-ext filters, keep order)
- [x] File/dir flag helpers + `--flag=value` behavior
- [x] Options to disable default commands or rename them

## P3 Parity (Help Output Fidelity)

- [x] Show default values in help
- [x] Stable sorting options (disableSortCommands/disableSortFlags)

## Plan

- [x] Define alignment scope (must/optional/unsupported) in COMPAT.md
- [x] Feature gap inventory vs Cobra
- [x] Phased rollout (example + CTest per gap)
- [x] Compatibility and stability (error/output consistency, edge cases)
- [x] Docs wrap-up (README + docs build)

## Code Review Findings (2026-11-02)

Filed from docs/review/2026-11-01-code-review.md. P1 (JSON config stack overflow) fixed in eccbea9.

- [x] `-v-2` count shorthand parses a negative increment (parser.hpp:1376-1398): the `-v<digits>` count form accepts a leading `-`, so `-v-2` records `-2` and `getCount` can go negative. pflag rejects this; reject a negative remainder. Fixed in commit 45463e3 · parser.hpp:1384-1400 · tests/branch_parser.cpp:1020-1023 · 569/569 CTest green (normal + ASan/UBSan)
- [x] Completion scripts interpolate the root command name and subcommand/flag word lists without shell escaping (command.hpp:3391-3511 (bash/zsh), 4172 (fish), 4330 & 4412 (powershell)). `bashCaseLabel`/`escapeDoubleQuotes` cover case labels and descriptions, but `rootName` and `joinWords(...)` are raw. Only reachable with command names containing shell metacharacters (developer-controlled); escape or document the assumption. Fixed in commit 75cb315 · command.hpp:3267-3385 · tests/completion_coverage.cpp:908-975 · CMakeLists.txt:320-372 · 569/569 CTest green (normal + ASan/UBSan) · coverage: line 100% (4177/4177); new helper branches 100%, only pre-existing TERM/TTY branches uncovered

## 审核发现

- [x] 审核发现：help 输出回归 —— `escapeDoubleQuotes` 现额外转义 `\` `$` 反引号（command.hpp:3267-3275），但同一函数仍被 `formatFlagForHelp` 复用渲染字符串 flag 默认值（command.hpp:2286），导致默认值含 `$`/`\`/反引号时 `--help` 显示多余反斜杠（如 `C:\path` 显示为 `"C:\\path"`）；现有测试无此类默认值用例，回归未被捕获 Fixed in commit fe1aa61 · include/clasp/command.hpp:2286,2653,3267-3288 · tests/coverage_final.cpp:250-283 · 569/569 CTest green
- 不属实：bash 补全脚本经 source 加载不触发 histexpand（实测 `source "foo!bar"` 在交互式 bash 正常执行）
- [ ] 审核发现：fish `-n` 条件的 `$` 转义不生效 —— `joinFishWords` 把 `$` 转义为 `\$`（command.hpp:3348-3354、4404），但 fish 会把 `-n` 条件字符串再作为命令求值，反斜杠被外层双引号解析消耗，子命令/别名名中的 `$VAR` 仍在运行时展开（既有行为，本次修复未覆盖该路径）
- [ ] 审核发现：死代码 —— `joinWords`（command.hpp:3330）在本次改动后无调用点，`escapeFishSingleQuoted`（command.hpp:3319）定义后从未被调用，建议删除或接入
- [ ] 审核发现：测试缺口 —— 新增补全转义测试仅做子串存在断言（tests/completion_coverage.cpp:907-975），未实际执行 bash/zsh/fish/powershell，故 zsh `#compdef "..."` 的引号解析、fish `-n` 运行时行为均未被端到端验证

## 审核发现（c48e9cff..HEAD 复核 · 已构建并运行 569/569 CTest 全绿 · 使用真实 bash/zsh/fish 实测）

- [ ] 审核发现：fish 静态补全 `-n "__fish_seen_subcommand_from <sub>"` 条件在子命令名含单引号 `'` 时解析失败 —— `escapeFishDoubleQuoted` 不转义 `'`（command.hpp:4415），但 fish 会把 `-n` 字符串再作为 fish 代码求值（实测报 `Unexpected end of string, quotes are not balanced`），导致含撇号子命令（如 `don't`）的局部 flag 补全整体失效；这是本次新增 fish 转义路径未覆盖的缺口，比既有 `$` 运行时展开发现影响面更广
- [ ] 审核发现：fish `-d "desc"` 描述中的反引号被过度转义 —— `escapeDoubleQuotes`（command.hpp:4403、4418）现转义 `` ` ``，但 fish 双引号内 `\`` 是字面反斜杠+反引号（实测 `"a\`b"` 输出 `a\`b`），含反引号描述在 fish 补全中多出一个反斜杠（纯显示层小瑕疵，bash 用例不受影响）
- [ ] 审核发现：`escapeBashDoubleQuoted`（command.hpp:3298-3306）与 `escapeDoubleQuotes`（command.hpp:3268-3276）逐字节相同，属重复实现，建议合并以免后续转义集漂移
- [ ] 审核发现：本轮修复未改变既有开放性结论 —— bash 双引号未转义 `!`（histexpand 下实测 `!b: event not found`）、fish `-n` 中 `$` 运行时展开、`joinWords`/`escapeFishSingleQuoted` 死代码、补全测试仅做子串断言未真正执行 shell；均在上一轮已登记，本轮仅补充实测证据，未复述
