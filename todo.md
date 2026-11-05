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
- [x] 审核发现：fish `-n` 条件的 `$` 转义不生效 —— `joinFishWords` 把 `$` 转义为 `\$`（command.hpp:3348-3354、4404），但 fish 会把 `-n` 条件字符串再作为命令求值，反斜杠被外层双引号解析消耗，子命令/别名名中的 `$VAR` 仍在运行时展开（既有行为，本次修复未覆盖该路径） Fixed in commit 4339438 · include/clasp/command.hpp:4421 · tests/completion_coverage.cpp:1014-1020 · 569/569 CTest green
- [x] 审核发现：死代码 —— `joinWords`（command.hpp:3330）在本次改动后无调用点，`escapeFishSingleQuoted`（command.hpp:3319）定义后从未被调用，建议删除或接入 Fixed in commit 4339438 (joinWords removed; escapeFishSingleQuoted now used by fish -n condition) · include/clasp/command.hpp:2659,3362,4421 · 569/569 CTest green
- [x] 审核发现：测试缺口 —— 新增补全转义测试仅做子串存在断言（tests/completion_coverage.cpp:907-975），未实际执行 bash/zsh/fish/powershell，故 zsh `#compdef "..."` 的引号解析、fish `-n` 运行时行为均未被端到端验证 Fixed in commit 38187d0 · tests/completion_coverage.cpp:1005-1093 (testShellEscapingEndToEnd) · 569/569 CTest green

## 审核发现（c48e9cff..HEAD 复核 · 已构建并运行 569/569 CTest 全绿 · 使用真实 bash/zsh/fish 实测）

- [x] 审核发现：fish 静态补全 `-n "__fish_seen_subcommand_from <sub>"` 条件在子命令名含单引号 `'` 时解析失败 —— `escapeFishDoubleQuoted` 不转义 `'`（command.hpp:4415），但 fish 会把 `-n` 字符串再作为 fish 代码求值（实测报 `Unexpected end of string, quotes are not balanced`），导致含撇号子命令（如 `don't`）的局部 flag 补全整体失效；这是本次新增 fish 转义路径未覆盖的缺口，比既有 `$` 运行时展开发现影响面更广 Fixed in commit 4339438 · include/clasp/command.hpp:3362,4421 (escapeFishSingleQuoted wraps -n condition) · tests/completion_coverage.cpp:1014-1015 · 569/569 CTest green
- [x] 审核发现：fish `-d "desc"` 描述中的反引号被过度转义 —— `escapeDoubleQuotes`（command.hpp:4403、4418）现转义 `` ` ``，但 fish 双引号内 `\`` 是字面反斜杠+反引号（实测 `"a\`b"` 输出 `a\`b`），含反引号描述在 fish 补全中多出一个反斜杠（纯显示层小瑕疵，bash 用例不受影响） Fixed in commit 236656a · include/clasp/command.hpp:4393,4408 (fish -d uses escapeFishDoubleQuoted) · tests/completion_coverage.cpp:1016-1017 · 569/569 CTest green
- [x] 审核发现：`escapeBashDoubleQuoted`（command.hpp:3298-3306）与 `escapeDoubleQuotes`（command.hpp:3268-3276）逐字节相同，属重复实现，建议合并以免后续转义集漂移 Fixed in commit e94e193 · include/clasp/command.hpp:3298-3300 (escapeBashDoubleQuoted delegates to escapeDoubleQuotes) · 569/569 CTest green
- [x] 审核发现：bash 端到端用例断言形同虚设 —— 管道退出码由末尾 `echo SRC_OK` 决定，`source` 失败后 `echo` 仍成功故整条命令返回 0，无法检出 histexpand/语法错误 Fixed · tests/completion_coverage.cpp:1044-1048 (source && echo || exit 1 · 实测 bad source exit=0→exit=1) · 569/569 CTest green
- [x] 审核发现：`::rmdir(dir)` 在写入 comp.bash/comp.zsh/comp.fish 之后调用必然失败（目录非空）且返回值未检查，每次运行向 /tmp 泄漏 `clasp_e2e_<pid>` 目录及其脚本文件 Fixed · tests/completion_coverage.cpp:1080 (std::filesystem::remove_all) · 569/569 CTest green
- [x] 审核发现：临时目录硬编码 `/tmp` 且路径基于 pid 可预测、`mkdir` 返回值未检查，随后又以当前用户身份执行其中脚本，存在预创建目录/符号链接攻击及陈旧残留文件被 source 的风险 Fixed · tests/completion_coverage.cpp:1021-1031 (std::filesystem::temp_directory_path + mkdtemp 0700 + 失败检查) · 569/569 CTest green
- [x] 审核发现：fish 端到端用例仅验证脚本可被 source（解析通过），从未实际调用 `__fish_seen_subcommand_from` 校验条件二次求值结果（`don't` 应匹配、`$x` 不应展开），本次修复的核心语义未被运行时断言 Fixed · tests/completion_coverage.cpp:1060-1078 (fish -N wrapper: complete -C 驱动 -n 条件，apos/literal/dollar 三向断言) · 569/569 CTest green
- [x] 审核发现：zsh 用例 `compinit` 向 `$HOME` 写入 `.zcompdump` 污染用户目录，且 `compinit` 失败被 `; source` 掩盖，仅 source 失败才会令测试失败 Fixed · tests/completion_coverage.cpp:1053-1059 (ZDOTDIR=隔离临时目录 && compinit && source) · 569/569 CTest green
- 不属实：`<cstdio>` 提供 `std::stderr` 与 `std::fflush`（tests/completion_coverage.cpp:24-26），且在 MSVC Debug 分支内被调用，并非多余头文件
- [x] 审核发现：zsh 端到端用例的 `ZDOTDIR` 隔离无效 —— `ZDOTDIR="$dir" autoload -Uz compinit && compinit ...` 的前缀赋值只对 `autoload` 内建生效，随后执行的 `compinit` 并不继承它，实测 `compinit` 仍向 `$HOME` 写入 `.zcompdump`（`$ZDOTDIR` 在命令后为空），与注释"cannot write .zcompdump into $HOME"的宣称相悖；上一轮 todo.md 标记该点"Fixed"为时过早 Fixed · tests/completion_coverage.cpp:1055-1066 (export ZDOTDIR + HOME 重定向，compinit 后断言 .zcompdump 落在隔离目录且 HOME 干净) · 569/569 CTest green
- [x] 审核发现：fish 端到端用例未覆盖命令替换型子命令名 —— 静态用例的子命令 `sub'$x`y\z`（含反引号，tests/completion_coverage.cpp:944）仅做子串断言，e2e 运行时测试（1062-1083）只验证 `don't`/`$x`，未验证反引号/`(...)`/`$(...)` 子命令名在 `-a`/`-n` 二次求值下不被当作命令替换执行；本轮 `-a` 修复依赖"每个词被双引号包裹使其成为字面量"这一未受运行时保护的假设（实测当前实现安全，属覆盖缺口，一旦双引号包裹回归将静默失效）Fixed · tests/completion_coverage.cpp:1000-1013,1067-1103 (新增 `` `cmd` ``/`$(cmdx)`/`(paren)` 别名 + 标记函数断言 -a 与 -n 二次求值均保持字面量) · 569/569 CTest green
- [x] 审核发现：`std::filesystem::remove_all(dir, ec)` 的返回值与 `ec` 未检查 —— 若清理失败（权限/竞态），临时目录静默残留，违背本提交"修复 /tmp 泄漏"的初衷 Fixed · tests/completion_coverage.cpp:1123-1126 (断言 remove_all 返回非 0 且 ec 无错) · 569/569 CTest green
- [x] 审核发现：bash 端到端用例"histexpand"表述与实际不符 —— 生成的 bash 脚本不含 `!`，交互模式的 history expansion 实际不会触发；新管线（`source && echo SRC_OK || { echo SRC_FAIL; exit 1; }`）确实能检出 source 失败（实测坏脚本 exit=0→非 0），但注释/提交信息"detect histexpand error"的表述高估了覆盖范围 Fixed · tests/completion_coverage.cpp:1042-1048 (注释改为仅检出语法错误，不宣称 histexpand) · 569/569 CTest green
- [x] 审核发现：bash 端到端断言消息 "script sources without histexpand error"（tests/completion_coverage.cpp:1056）仍宣称检出 histexpand，但本提交同批重写的注释（1048-1052）已明确"生成的 bash 脚本不含 `!`，history expansion 实际不触发"；断言消息未同步改为"仅检出语法错误/source 失败"，表述与自身注释相矛盾 Fixed · tests/completion_coverage.cpp:1057 (断言消息改为 "script sources without syntax error") · 569/569 CTest green
- [x] 审核发现：`std::filesystem::create_directories(zshHome, ec)` 的返回值与 `ec` 未即时断言（tests/completion_coverage.cpp:1068）——若创建失败，后续 `zsh -fc` 的 `HOME`/`ZDOTDIR` 指向不存在目录，错误仅被 1073 行 `expect` 间接且不明确地暴露，无法区分"compinit 失败"与"HOME 目录未创建" Fixed · tests/completion_coverage.cpp:1069-1070 (断言 create_directories 返回 true 且 ec 无错) · 569/569 CTest green
- [x] 审核发现：fish `-n`/`-a` 中转义形式 `"\\$(cmdx)"`、`"\\$x"`（tests/completion_coverage.cpp:1022 断言固定此文本）仅在 fish `complete` 参数求值上下文下成立——普通 fish 双引号求值下 `"\\$(cmdx)"` 实测会执行命令替换（`cmdx` 被调用），`"\\$x"` 实测得到 `\`+`$x` 展开值而非字面量 `$x`；当前生成脚本正确只因 fish `complete` 的 `-n`/`-a` 求值不执行命令替换（fish 4.2.1 实测），该测试仅经 `complete -C` 间接验证此上下文特性，转义安全性对 fish 版本与求值上下文敏感 Fixed · include/clasp/command.hpp:3336-3378 (joinFishWords/joinFishConditionWords 改用单引号字面量 `'...'`，fish 单引号内 `$`/`` ` ``/`()` 不做任何展开) + tests/completion_coverage.cpp:1021-1024,1089-1114 (断言与 e2e 校验单引号字面量语义) · 569/569 CTest green
- [x] 审核发现：fish 补全单引号转义回归 —— 子命令名以奇数个反斜杠结尾（如 `a\`）时 `escapeFishSingleQuotedContent` 不转义反斜杠，生成的 `-a` 值经第一次求值得到 `'a\' 'help'`，fish 二次求值把紧邻闭合引号的 `\'` 当作转义引号导致字符串未闭合，实测 fish 4.2.1 下 `complete -c "t" -f -a '\'a\\\' \'help\''` 报 "incomplete token"、`-n` 报 "quotes are not balanced"，补全脚本无法加载；98c27980 的旧双引号实现处理 `a\` 正常（回归）（include/clasp/command.hpp:3374） Fixed · include/clasp/command.hpp:3365-3377（content escaper 补反斜杠转义；外层 wrapper 本就双写反斜杠，内层不补则尾随 `\` 吞闭合引号）+ fish 4.2.1 实测：`bs\` 出现在 `complete -C` 子命令列表、经 `-n` 条件二段求值后 flag 正常给出 · 569/569 CTest green
- [x] 审核发现：测试覆盖缺口 —— metachar 与 e2e 用例的子命令名均不以反斜杠结尾（`sub'$x`y\z` 反斜杠在中部，`don't`/`$x`/`` `cmd` ``/`$(cmdx)`/`(paren)` 无尾随反斜杠），未覆盖"奇数尾随反斜杠"回归，故本轮断言未能捕获上述缺陷（tests/completion_coverage.cpp:926） Fixed · tests/completion_coverage.cpp:1007-1010（e2e 树新增 `bs\` 别名）、1100（`contains` 断言）、1121-1125（`complete -C` 经 -n 条件给 flag）；metachar 中部反斜杠断言同步为新转义（tests/completion_coverage.cpp:932） · 569/569 CTest green

- [x] 审核发现：代码块内的 `code` 仍被 VitePress 默认样式设为 `white-space: pre`，覆盖从 `pre` 继承的换行设置，长代码行不会按预期换行（docs/.vitepress/theme/custom.css:57）。 — 不属实：HTTP 渲染探针（dist 按 base `/clasp/` 起服后取 computed style，guide/getting-started）实测 `pre` 与 `pre code` 均为 `pre-wrap`，长代码行已换行（docs/.vitepress/theme/custom.css:57-61）
- [x] 审核发现：文档内容宽度规则的选择器优先级低于 VitePress 带作用域属性的默认规则，含侧栏目录的页面仍受 688px 限制，无侧边栏页面的宽度覆盖也不生效（docs/.vitepress/theme/custom.css:64）。 — 不属实：渲染探针实测 `.VPDoc.has-aside .content-container` 与 `.VPDoc .container > .content` computed max-width 均为 960px，`!important` 已生效（docs/.vitepress/theme/custom.css:62-70）
- [x] 审核发现：`max-width: 960px !important`（3 处）用 `!important` 硬压 VitePress 默认宽度，属级联反模式，会覆盖一切后续响应式/打印/主题样式；尤其 `.VPDoc.has-aside .content-container` 在有右侧目录时本应收窄预留 aside 空间，强制 960px 可能在中等宽度视口造成内容与目录挤压或横向溢出，且后续无法再覆盖（docs/.vitepress/theme/custom.css:62,65,68） — 不修（登记取舍）：VitePress 默认宽度规则带 scoped 属性选择器、特异性更高，非 `!important` 无法覆盖；浏览器默认样式表不受 `!important` 影响，用户样式仍可再压；1440px 视口渲染探针无挤压/溢出。此为舰队三仓统一约定（chame/cage 同块），如后续引入窄断点适配再整体重构（docs/.vitepress/theme/custom.css:62-70）
- [x] 审核发现：双段转义不变量依赖两份手工同步的代码 —— escapeFishSingleQuoted（外层包装，4421/4441 使用）与 escapeFishSingleQuotedContent（内层，3345/3360 使用）的转义循环经本次修改后逐字符相同但仍是两份独立实现，外层未委托内层（可写为 `"'" + escapeFishSingleQuotedContent(s) + "'"`）；将来只改其一会使 `-a`/`-n` 的双段编码 round-trip 静默破坏，重新打开本次修复的"尾随反斜杠吞闭合引号/词表损坏"类缺陷且现有断言未必能拦截（include/clasp/command.hpp:3379-3389） Fixed · include/clasp/command.hpp:3379-3383（外层委托内层 `escapeFishSingleQuotedContent`，双实现归一为单一转义规则）+ 负向验证（临时移除反斜杠转义即 e2e fish 断言 fail）+ 569/569 CTest green
- [x] 审核发现：测试覆盖缺口 —— 偶数个相邻反斜杠（如别名 `bs\\`、`a\\b`）在旧实现下被静默损坏（join 输出 `'bs\\'`，二次求值 `\\`→`\`，两个变一个），本次补反斜杠转义已顺带修好但无任何断言固定；新增用例仅覆盖奇数尾随 `bs\`（中毒令牌，loud 失败）与单个中部反斜杠 `\z`（fish 单引号本就保留、新旧实现皆通过），若未来转义被"只处理尾随反斜杠"式部分实现替代，偶数对回归不会被现有 metachar/e2e 断言捕获（tests/completion_coverage.cpp:1010） Fixed · tests/completion_coverage.cpp:1010-1016（e2e 树新增 `bs\\\\`/`a\\\\b` 别名）、1034（-n 条件词断言）、1107-1112（`contains` 断言）、1138-1145（`complete -C` 经 -n 条件给 flag）+ 负向验证（移除反斜杠转义即 `e2e fish: -n condition` fail）+ 569/569 CTest green

- [x] 审核发现：本轮 diff（c8c418cc..HEAD = f80cba9）经 fish 4.2.1 实测 + completion_coverage_test 全绿确认修复正确、未引入新问题；唯余两条已开放待办（双段转义双实现需手工同步 include/clasp/command.hpp:3379-3389、偶数反斜杠覆盖缺口 tests/completion_coverage.cpp:1010），不重复登记 Fixed · 两条待办均已在本轮解决（双实现归一 include/clasp/command.hpp:3379-3383；偶数反斜杠断言 tests/completion_coverage.cpp:1010-1145）+ 569/569 CTest green
- [x] 审核发现：本轮 diff 未见问题（f80cba94..HEAD）。 审核通过，无需改动
