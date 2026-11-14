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

## 审核修复批 2026-11-12（巡检单 2026-10-20 · /home/cui/audit-batch5-6repos-2026-11-11.md clasp 节）

- [x] 审核修复 P0-1：内置 version 拦截不可关闭 —— `resolveForExecution` 对 `version` token 无条件拦截（用户注册的 version 子命令被吞），且 root 未设 version 时静默退出码 0 无任何报错。修复：拦截仅当 `!findSubcommand("version") && !resolvedVersion().empty()`（用户子命令/别名优先）；`run()` 位置参数拦截同样加两重守卫，未设 version 时 `version` 落回普通命令解析（非 runnable root 报 unknown command 退出 1） Fixed in commit 9f63a1c · include/clasp/command.hpp:1433-1437,3141-3147 · tests/command_gaps.cpp:406-453（red-first 验证：移除修复后 user/aliased version action output、builtin version not printed、version without version set rejected/errors 五断言 fail）· 569/569 CTest green
- [x] 审核修复 P0-2：`--` 被当 flag 值消费（parser.hpp:149/895）——实测核对 pflag/cobra 源码（spf13/pflag parseLongArg/parseSingleShortArg、spf13/cobra stripFlags）并跑 go 1.27 live 探针：**pflag 本身就把 `--` 当普通 flag 值消费**（`--flag -- x` → flag=`"--"`、x 为普通参数；NoOptDefVal flag 则不吃 `--`），parser.hpp:149/895 与 pflag 逐点一致，审核该半条「pflag/Cobra 不会」**不属实**。真正错位在命令解析层 `skipFlagValueIfNeeded`：NoOptDefVal flag 后跟 `--` 时 clasp 把 `--` 当值跳过并继续解析子命令（`app --opt -- sub` → sub 被解析），而 Cobra stripFlags 在 `--` 处终止扫描（`app --opt -- sub` → root 以位置参数 `sub` 运行；live 探针证实 cmd=root args=[sub] opt=DEF）。修复：执行与补全两份解析器对 NoOptDefVal flag 不再消费 `--`（`--` 终止命令解析）；非 NoOptDefVal flag 照旧消费 `--`（与 Cobra stripFlags 一致，`app --plain -- sub` → sub 运行） Fixed in commit e9c7175 · include/clasp/command.hpp:2739,2754（补全解析器）3105,3122（执行解析器）· tests/command_gaps.cpp:578-580,622-668（red-first：noOpt flag -- keeps root completion target、long/短名/短组 noOpt flag: root runs、token stays positional 六断言 fail）· 569/569 CTest green · line 覆盖率 100%（4673/4673）
- [x] 审核修复 P0-3：normalizeFlagKeys 与注册键不一致（parser.hpp:743-815）——registerFlag 以原始（未 normalize）flag 名写入 aliases_/kinds_/defaults_/noOptDefaults_/countKeys_/bytesKeys_/ipKeys_/ipMaskKeys_/cidrKeys_/ipNetKeys_/urlKeys_/int8/16/32Keys_/uint8/16/uintKeys_/ipSliceKeys_/bytesBase64Keys_/knownKeys_，而解析期 lookup 经 normalizeKey 归一后再查表，自定义 normalize（如下划线→连字符）后声明默认值静默丢失、类型校验失效、注解 flag 退化、子命令同名值误当命令。修复：registerFlag 对声明的 long/short 名先经 normalizeKey 归一后再写各表（pflag 定义期归一语义）；命令解析层新增 `flagMatchesNormalized` 助手，四份 flagInfoLocal/flagInfoSubtree 比较点（补全+执行各两份）同时接受原始与归一拼写（raw 命中短路，避免多余 hook 调用）。防御性说明：normalizeKey 在 ctor 注册期被调用，故用 try/catch 包裹——抛异常的 hook 回退原始名，保 `testThrowingNormalizeKey`「无 flag token 时 ctor 完成」契约（该 hook 异常仍经解析路径转义，block1 语义不变） Fixed in commit 7e16e82 · include/clasp/parser.hpp:743-808 · include/clasp/command.hpp:1902-1910（flagMatchesNormalized）2687,2708,3053,3074（四比较点）· tests/command_gaps.cpp:303-379,935（red-first：移除修复后解析下划线 flag/值达 action/dash 拼写/默认保留/注解类型校验/子命令同名值/补全识别 十一断言 fail）· 569/569 CTest green · line 覆盖率 100%（4686/4686，branch 99% 为既有 shell_escape.hpp:72 漂移，未扩大）
- [x] 审核修复 P1-4：getBytesBase64 吞解码失败静默返回空（parser.hpp:528）——getBytesBase64 丢弃 `tryDecodeBase64` 的 bool 返回值：CLI 值经 normalizeValue（parser.hpp:653-657）与 config/env（setExternalValuesChecked/setExternalValuesMultiChecked）均严格校验，故解码失败仅可能来自 (a) withBytesBase64Flag 声明了非 base64 默认值（pflag 的 bytesBase64 默认本就是已解码字节，不可表达非法默认）或 (b) 未校验的 setExternalValues/setExternalValuesMulti 注入；且 tryDecodeBase64 逐 quad 推进，失败时 out 保留此前已解码的前缀字节，getBytesBase64 返回的是部分垃圾而非空。修复：registerFlag 对 bytesBase64 注解 flag 校验默认值——非法即置 ok_=false + error_（run() 经 parser.ok() 报 `invalid argument "<def>" for "<flag>"`，静默不再）；ctor 注册循环后 `if (!ok_) return` 使声明错误优先；getBytesBase64 解码失败清空 out（防御，注入路径不再吐部分前缀） Fixed in commit 876000f · include/clasp/parser.hpp:65-67（ctor 早退）528-535（getBytesBase64 防御清空）825-833（默认值校验）· tests/command_gaps.cpp:384-429,988（red-first：移除修复后 invalid default is rejected / rejection names invalid default / unchecked invalid base64 yields no partial bytes 三断言 fail）· 569/569 CTest green · line 覆盖率 100%（4692/4692，branch 99% 为既有漂移，未扩大）
- 不属实：P1-5 base=0 八进制陷阱 `--port 010`→8（value_parse.hpp:47/66）——`strtoll`/`strtoull` 以 base 0 自动识别进制，`010` 按八进制→8。但 pflag 同样如此：live go 探针（spf13/pflag v1.1.0）`--port=010`→8、`--u=010`→8、`--i64=010`→8、`--port=0x10`→16，即 pflag 的 `strconv.ParseInt(s, 0, ...)` 也是 base 0 自动识别（前导 0 八进制、0x 十六进制），clasp 与 pflag 逐点一致，非 clasp 缺陷。且 clasp 的 base-0 十六进制行为被既有测试显式固定（tests/branch_value_types.cpp:97,107「signed/unsigned base-0 hex」0x10→16、tests/value_parse.cpp:83,119 0xFF→255、tests/parser_extra_coverage.cpp:719,733,747 `--size/--port/--value 0xFF`→255），改 base 10 会破坏十六进制 parity。附注：pflag 额外接受 `0b` 二进制与 `1_000` 下划线分隔（Go 1.13+），clasp 的 strtoll 不支持——属独立功能缺口，非本条审计项，如需对齐另立条目。
- [x] 审核修复 P1-6：NoOptDefVal 消费下一 token 三处不一致（parser.hpp:138-154/911-927 + command.hpp 执行/补全解析器四处）——live go 探针（spf13/pflag v1.1.0 + cobra）：pflag 的 NoOptDefVal flag **永不**消费下一 token（`--opt x`→opt=DEF、x 为普通参数；`--opt sub`→opt=DEF、args=[sub]；`--opt --verbose`→opt=DEF、verbose=true），值 flag 则无条件消费下一 token。clasp parser 的 long/short 形式仅在「下一 token 是 flag」时用 noOpt 默认，遇到普通 token（如 `--mode manual`）仍把它当值吞掉，与 pflag 相悖；命令层四份解析器（补全 long/短组 + 执行 long/短组）则又各有第三种行为（有条件跳过），三处不一致。修复：parser 两处改为无条件 noOpt 默认（下一 token 一律留在位置参数），命令层四处 resolver 改为无条件跳过值消费（长形式 `if (info->hasNoOptDefault) return;`、短组 `if (info->hasNoOptDefault && pos + 1 == token.size()) return;`）。附带更新 5 处固定旧行为的既有测试：branch_parse_paths.cpp:70-84、parser_gaps.cpp:293-298、completion_protocol.cpp:443,452,453,456,457（非子命令词不再被吞，rc 0→1 如同行 446 mid-group 既有断言）、parser_extra_coverage.cpp:695-711（改断言 mode:auto + args:1）、cmake/tests.cmake:628-629（`--mode manual`→`--mode=manual`，pflag 显式赋值形式）与 1855-1856 Fixed in commit 15323d4 · include/clasp/parser.hpp:138-154,911-927 · include/clasp/command.hpp:2748-2751,2761-2765（补全）3111-3115,3125-3133（执行）· tests/command_gaps.cpp testNoOptFlagNeverConsumesNext（12 断言；red-first：移除修复后 4 断言 fail：long/short 普通token不吃、短组组尾不吃、执行层 root 收到 positional）· 569/569 CTest green（build-lay -j8 与 build-cov -j1 双绿）· line 覆盖率 100%（4640/4640，branch 99% 为既有 shell_escape.hpp:72 漂移，未扩大）
- [x] 审核修复 P1-7：短组中间带值 flag completion 错位（command.hpp 分离值补全探测）——补全的分离值探测只认 `words.back()` 为完整 flag token（`resolveFlagNameForValueCompletion(prev)`），短组 token（如 `-fc`）解析失败直接落入子命令补全；而解析器语义是组尾成员吃下一个词（中段成员吃余下字符作值）。live cobra 探针（spf13/cobra，`__complete` 驱动）证实 cobra 取组最后一字符作 flag 名（`-fc`→color 值候选 red/green；`-cc`→同，naive 末字符规则；`-cf`→:0 Default 中段不触发；`-fl`/`-fn`→:0 Default noOpt/count 组尾不触发；`--level`/`-l`/`--tally`→:0 Default 长形式 noOpt/count 也不触发）。修复：探测键改为长形式取原词、短形式取末字符（`std::string("-") + prev.back()`，与 cobra 同规则）；expectsValue 除 bool 外再排除 count 注解与 noOpt 默认 flag（两者从不消费下一词，与 P1-6 语义及 cobra NoOptDefVal 守卫一致）；`--flag=` 形式不受 noOpt 守卫影响（cobra flagWithEqual 同） Fixed in commit 69f1957 · include/clasp/command.hpp:2895-2916 · tests/completion_protocol.cpp testValueCompletionShortGroupTail（12 断言：组尾值成员给候选/前缀过滤/重复成员仍触发/中段不触发/noOpt组尾·长短形式/count长形式不触发/eq形式保持/单短名不变/bool尾不触发；red-first：移除修复后 6 断言 fail）· 569/569 CTest green（build-lay -j8 与 build-cov -j1 双绿）· line 覆盖率 100%（4645/4645，branch 99% 为既有漂移，未扩大）
- 不属实：P1-8 IPMask 拒 0.0.0.0（pflag 允许）——clasp 全链路实测均**接受** 0.0.0.0：tryParseIPMask("0.0.0.0")→OK（parser_value.hpp:396-399 连续位检查 `inv & (inv+1)`，inv=0xFFFFFFFF 时 inv+1=0x100000000、低 32 位为 0，AND=0 通过）、CLI `--mask 0.0.0.0`/`--mask=0.0.0.0` rc=0、默认值 0.0.0.0 正常、setExternalValues/setExternalValuesChecked 无错；且既有测试早已固定该行为（tests/branch_value_types.cpp:287「mask zero ok」，569 全绿中）。pflag 侧 live 探针（spf13/pflag v1.0.10）同样 OK 00000000，两侧一致，审核该条不属实。附注（反向差异）：pflag 的 ParseIPv4Mask（ipmask.go:33-57）无连续位检查，`255.0.255.0`/`1.2.3.4`/hex 形式 `ffffff00` 全部接受；clasp 要求连续位（command.hpp:432 注释明示「contiguous bits」为既定校验，tests/branch_value_types.cpp:289,291 固定拒绝）——clasp 更严，属既定设计取舍，非本条审计所指缺陷，如需放宽对齐另立条目。

### P2 登记待办（巡检单 2026-10-20：仅登记不修，按批次另行安排）
- [x] P2：count 语义混用（count 注解 flag 的 getFlag<int>/getCount 两条读取路径语义需统一梳理）——pflag 实测（spf13/pflag CountP 经 Set 预置默认值 3）：`-v`→4、`-vvv`→6，即 Count 在**声明默认值之上累加**，从不重置默认。clasp 旧 getCount 忽略声明默认（base 恒 0）：默认 3 的 count flag `-v` 后 getCount→1、occurrences 语义与值语义混淆。修复：getCount 对 count 注解 flag 以声明默认值为 base，在其上累加各次出现（无注解 flag 保持旧 fallback 语义）；新增 isCountFlag(key) 公开 count 注解判定；getFlag<T>「末值/声明默认」语义保持不变（count 注解 flag 的各次出现记录 "1"/显式增量，末值即最近一次增量，读取路径统一经 getCount）。 Fixed in commit 9bb5385 · include/clasp/parser.hpp:217-221（isCountFlag）251-263（getCount base 累加）· tests/parser_gaps.cpp testCountAccumulatesOverDefault（9 断言：零默认/默认保留/单次-vvv 增量/显式增量/非注解 flag 回退；red-first：临时将 base 恒 0 后 3 断言 fail）· 569/569 CTest green（build-lay）
- [x] P2：markFlag* 系列对未知/不匹配 flag 静默 no-op，注册期错误不暴露（可参照 P1-4 bytesBase64 默认校验的 fail-fast 先例）——markFlag* 系 10 个方法（Required/Hidden/Deprecated/Annotation/NoOptDefaultValue × 本地/持久）查找失败静默返回，markFlagFilename/Dirname（含 Persistent 变体）也无存在性检查；拼错 flag 名/把本地 mark 用在持久 flag 上（反之亦然）完全无症状。修复（沿用 P1-4 fail-fast 先例，因 API 返回 Command& 链式无法即时返回错误）：各 mark* 查找失败时记入新增 unmatchedMarkErrors_（`<方法名>: unknown flag "<名>"`），filename/dirname 包装以本地+持久双集合存在性检查（registerFlagCompletion 公共 API 保持不检查）；run() 入口先经 collectUnmatchedMarkErrors 汇总全树（子命令一并查）报告并退出 1（不显示 usage，属注册错误非输入错误）。既有测试 testFlagMutationOnUnknownFlag 原固定「mark 未知 flag 是 no-op rc==0」，按新契约改写。 Fixed in commit d29ba09 · include/clasp/command.hpp:969-1006（recordUnmatchedMark/flagExistsLocallyOrPersistent/10 mark* 方法）1014-1089（filename/dirname 检查）1391-1406（collectUnmatchedMarkErrors + run 入口）3259-3261（成员；move 构造/赋值同步）· tests/command_gaps.cpp testFlagMutationOnUnknownFlag（8 断言：未知 required/filename 报告+指名、集合不匹配报告+指名、匹配 mark 干净运行、子命令未知 mark 根运行报告；red-first：移除修复后 3 断言 fail——旧测试原固定 rc==0）· 569/569 CTest green（build-lay）
- [x] P2：enableColor 重复调用重复追加转义序列——enableColor 每次 addFlags=true 都向 persistentFlags_ 再注册一份 `--color`/`--color-theme`（probe 实测双调用后 help 中两 flag 各出现两次）；flagCompletionFuncs_ 同 key 覆盖无害，但重复 Flag 令 help/usage 输出重复行、effectiveFlags 含重复项。修复：注册前检查 alreadyInstalled（colorFlagsInstalled_ 已置或 persistentFlags_ 已含 `--color`/`--color-theme`，后者兼防用户自注册同名 flag 被重复追加），仅首次安装注册 flag 与 completion；重复调用仅更新 mode/theme 运行时与默认值（colorFlagsInstalled_ = addFlags 保持原语义） Fixed in commit 40edf33 · include/clasp/command.hpp:250-267 · tests/command_gaps.cpp testEnableColorIdempotent（7 断言：双调用 help 各列出一次/--color 仍解析校验/带参再启用仍单次（--color=never 规避 ANSI 码干扰计数）；red-first：移除修复后 3 断言 fail）· 569/569 CTest green（build-lay）
- [x] P2：.env 行内注释不剥离（`KEY=value # comment` 的尾部注释进入值）——实测证实（godotenv v1.5.1 live 探针，github.com/joho/godotenv parser.go extractVarValue）：未加引号的值**反向**扫描最后一个「前面是空白」的 `#`，其前全部保留（`KEY=value # comment`→`value`；`KEY=value#nospace`→`value#nospace` 保留；`KEY= # comment`→`# comment` 保留原文，因 `#` 前只有空白）；引号值把 `#` 挡在注释剥离之外（`KEY="value # not comment"`→`value # not comment`；`KEY="v" # c`→`v`）。修复：parseEnvLike 对未加引号值反向扫描剥离行内注释（新增 stripEnvComment + isCommentSpace，后者排除换行因逐行扫描）；引号值改为定位首个**未转义**的闭合引号后截断（`"a\"b"` 在末尾闭合而非转义引号处），闭合引号之后的注释随之丢弃；引号未闭合时保持原值（与原 no-op 行为一致，godotenv 报 unterminated 错误但 clasp 的宽松容错语义保留）。Fixed in commit 4f0a7c7 · include/clasp/detail/config_parse.hpp:31-33（isCommentSpace）39-46（stripEnvComment）48-87（parseEnvLike 引号/注释分支）· tests/config_parse_coverage.cpp:157-186 testEnvInlineComment（15 断言：7 例矩阵 接受+取值/空白值带注释保原文；red-first：stash 修复后 3 断言 fail——行内注释 case1/引号后注释 case4/制表符注释 case7）· 569/569 CTest green（build-lay -j8 与 build-cov -j8 双绿）· line 覆盖率 100%（include/clasp/detail/config_parse.hpp 379/379）
- 不属实：P2：YAML 首个冒号分割截断（值内冒号丢失）——实测 clasp parseYamlFlatten **已按首个冒号分割并完整保留值内冒号**：`url: http://example.com:8080/x`→key=`url`、value=`http://example.com:8080/x`（全部冒号在值内保留）、`now: 12:30:00`→`12:30:00`、`msg: a: b: c`→`a: b: c`、嵌套 `sec:`/`url: http://h:80/p`→key=`sec.url`、序列项 `- http://a:1/`→原样保留、单双引号包裹的值内冒号同样保留。首个冒号即分割点（config_parse.hpp:504 `content.find(':')`），无截断。审核该条不属实；若所指为「冒号后无空格的 key:value」或「键含冒号需引号」之类，则属未声明的额外语义，非现行实现缺陷。
- [x] P2：JSON 解析非严格（宽松接受非法 JSON）——实测确认 5 处违反 RFC 8259 的宽松接受（probe 对照）：前导 `+`（`{"sv": +5}` 被接受且值 `+5`）、前导零（`01`）、小数点无数字（`1.`）、缺整数部分（`-.5`）、字符串内**原始**控制字符（`"a<LF>b"`/`"a<TAB>b"` 均被接受，仅 `\n`/`\t` 转义形式才应合法）。既有矩阵 tests/config_parse_coverage.cpp:204 **显式固定了旧宽松契约**（`json: leading plus sign accepted`），属审核所指宽松点的直接来源。修复：parseNumberToken 改写为 RFC 8259 文法 `[ "-" ] int [ frac ] [ exp ]`，int 为 `0` 或 `1-9` 后接数字（无前导 `+`、无前导零、`-.5` 缺整数部分即 nullopt），frac 小数点后须 ≥1 位数字，exp 沿用原校验；parseScalarToken 路由去掉 `+`（`+5` 落到 nullopt 拒绝）；parseString 对未转义的 U+0000–U+001F 返回 nullopt（转义形式 `\\n`/`\\t` 不变）。`01` 解析为 `0` 后余 `1`，由外层分隔符检查（`}`/`,`）拒绝，与 RFC「两 token 缺分隔符」判定一致。Fixed in commit 0f8d0c0 · include/clasp/detail/config_parse.hpp:142-146（控制字符拒绝）168-192（数字文法）208（路由去 `+`）· tests/config_parse_coverage.cpp:200-211 testJsonBranchMatrix 矩阵翻转 `+5` 契约为拒绝 + 新增 8 例（前导零/小数点无数字/缺整数部分/数组内前导零/原始换行/原始制表符 均拒；`0`/`-0.5e-2`/转义控制字符 均收）· red-first：stash 修复后 7 个拒绝用例 14 断言 fail（含 `[error text]`），修复后 0 fail · 569/569 CTest green（build-lay -j8 与 build-cov -j8 双绿，后者重跑 2 次均绿）· line 覆盖率 100%（include/clasp/detail/config_parse.hpp 377/377；include/* 全头 100%，总体 99.2% 差额仅来自 tests/* 既有行）· 独立标准实现对照：15 例与 Python 3 `json` 模块（严格模式）判定逐例一致
- [x] P2：Command move 构造约 60 个成员手工罗列，新增成员易漏（维护风险，建议成员初始化器或复制再改）——实测成员实为 **72** 个（非审核所称约 60），两份清单（构造 init 列表与赋值体）当前逐项对齐、无缺漏，纯维护风险。且发现更严重的暗面：**全仓不存在任何 `operator=(Command&&)` 调用点**（无 vector&lt;Command&gt;、无 swap、无直接赋值），gcov 实测 37 个测试 TU 中赋值体全部为 `-`（未发射计数器）——移动赋值是**从未链接、从未测试的死代码**，其清单若漏一个成员永远无人能察觉（gcovr 的 100% 因未 ODR-use 的 inline 函数不发射行而静默跳过）。修复：两个 move 操作收敛为单一 `moveAllMembers` 成员表（`moveMember(&Command::成员, dst, src)` 72 行），构造 `{ moveAllMembers(*this, other); }`、赋值 `{ 守卫; moveAllMembers(*this, other); reparent 循环; }`——新增成员只可能加一处，两操作物理上不可能漂移；配套 testCommandMoveAssignment 使移动赋值**首次获得调用方**（整树移动：name/版本经 --version 输出/别名经 argv0 派发/flag 值/hidden 标记/help/子命令 reparent 派发 + 自赋值守卫），死代码变测试代码。Fixed in commit 028f9ea · include/clasp/command.hpp:75-90（注释+两个 move 操作）3115-3196（moveMember/moveAllMembers 单一表，紧跟成员声明区）· tests/command_gaps.cpp:1139-1218 testCommandMoveAssignment（12 断言）· red-first 双验证：从表中删 `version_` 一行→「carries version」fail；删自赋值守卫→exit 1（自移动损坏）；恢复后 0 fail · 569/569 CTest green（build-lay -j8 与 build-cov -j8 双绿，后者连跑 3 次均绿，一次 6-fail 为 -j8 瞬态）· line 覆盖率 100%（include/clasp/command.hpp 2599/2599，此前死代码行现全部有计数）
- 不属实：P2：gcov 产物文件入库（应加 .gitignore）——三重核实：①`git ls-files` 无任何 `.gcov`/`.gcda`/`.gcno`/`.info` 被跟踪（仅测试源文件名含 coverage 字样）；②`git log --all --diff-filter=A -- "*.gcov" "*.gcda" "*.gcno" "*.info" "*.gcov.json.gz"` 为空——历史上从未提交过任何覆盖产物；③`.gitignore` 早在 2026-09-14（commit 807186d「chore: add gcov artifacts to .gitignore and remove from tracking」，早于 2026-11-11 审计 2 个月）已含 `*.gcda`/`*.gcno`/`.gcov`/`*.gcov.json.gz`/`build-cov/`/`build_cov*/` 全部模式。审计当时看到的应是**未跟踪的工作区残留**（审计工具自产，从未 staged），不构成「入库」。顺手清理：`.gitignore` 中 `*.gcno`/`*.gcda`/`*.gcov` 三个模式重复列了两遍，去重为单份 Fixed in commit c1ac5d1 · .gitignore:51-54 · 569/569 CTest green（build-lay）
- [ ] P2：LCOV_EXCL 注释滥用（多处用于非排除目的的行尾注释，弱化排除标记可信度）

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
- [x] 审核发现：本轮 diff 未见问题（18c55024..HEAD）。 rev-list: `794b03e docs: mark audit pass for diff f80cba94..HEAD`；经 `git show` 确认该区间唯一提交仅修改 todo.md（docs-only），无代码/配置改动。
- [x] 审核发现：本轮 diff 未见问题（794b03ef..HEAD）。 rev-list: `68ae290 todo: tick clean audit for 18c55024..HEAD`；经 `git show` 确认该区间唯一提交仅修改 todo.md（docs-only），无代码/配置改动；569/569 CTest green（build-audit）
- [x] 审核发现：本轮 diff 未见问题（68ae290a..HEAD）。 rev-list: `bfded17 todo: tick clean audit for 794b03ef..HEAD`；经 `git show` 确认该区间唯一提交仅修改 todo.md（docs-only），无代码/配置改动；569/569 CTest green（build-audit）

## 源码分层分析 2026-11-07

### a) 抽象层清单

- 契约、构建与打包：定义 C++17、版本一致性、CMake target/install/export 与可选示例测试；代表模块 `CMakeLists.txt:2`、`cmake/claspConfig.cmake.in:3`。
- 命令树与应用模型：以 `Command`/`Flag` 承载子命令所有权、标志声明、回调和继承配置；代表模块 `include/clasp/command.hpp:69`、`include/clasp/flag.hpp:23`。
- 执行编排：解析执行节点，串联 parser、help/version、外部源、校验、钩子和动作；代表模块 `include/clasp/command.hpp:1376`、`include/clasp/command.hpp:3034`。
- Token 解析与类型校验：把 `argv` 变成有序显式值，校验/规范化基础类型和注解扩展类型；代表模块 `include/clasp/parser.hpp:44`、`include/clasp/parser.hpp:1125`。
- 外部源集成：读取进程环境与配置文件，扁平化映射并按优先级注入 parser；代表模块 `include/clasp/command.hpp:3673`、`include/clasp/command.hpp:4205`。
- 表现、导出与终端适配：生成 help/usage/version/Markdown/roff，并通过 color 适配终端能力；代表模块 `include/clasp/command.hpp:1494`、`src/color.cpp:14`。
- 补全协议与 Shell 生成：安装动态 complete 命令、生成四种 shell 脚本并统一上下文解析；代表模块 `include/clasp/command.hpp:4560`、`include/clasp/command.hpp:2815`。
- 消费验证与文档契约：用 examples、unit/CTest 与双语文档固定 API 和可见行为；代表模块 `examples/basic_example.cpp:7`、`tests/execute.cpp:7`、`docs/guide/index.md:3`。

### b) 超大文件候选

- [x] LAY-001 [P1] 拆分 include/clasp/command.hpp（4675 行）——命令模型、执行编排、外部源解析、表现导出和补全协议共享一个类/头，状态与关注点过度耦合（include/clasp/command.hpp:3189）。 Fixed in commit 6d56799 · command.hpp 4675→3980 行：配置文件扁平解析（.env/.json/.toml/.ini/.yaml 六个 parser + ConfigRaw/JsonParse 深度防溢出）抽到 include/clasp/detail/config_parse.hpp（534 行），补全脚本 shell 转义 15 个函数抽到 include/clasp/detail/shell_escape.hpp（193 行），applyExternalSources/补全生成改经 detail:: 调度，公开 API 与行为不变 · 569/569 CTest green
- [x] LAY-002 [P1] 拆分 include/clasp/parser.hpp（1478 行）——token 解码、内存状态、访问优先级和十余种类型校验器同在一个泛型类（include/clasp/parser.hpp:1125）。 Fixed in commit 1561b7a · parser.hpp 1478→947 行：类型/值词汇（parse<T>/parseBool、ValueKind 判别、bytes/IPv4/IPv6/IP/CIDR/IPMask/URL/CSV 校验与规范化）抽到 include/clasp/detail/parser_value.hpp（572 行），Parser 保留 `using Kind = detail::ValueKind` 别名，调用点经 detail:: 派发 · 569/569 CTest green
- [x] LAY-003 [P2] 拆分 CMakeLists.txt（2438 行）——根构建、安装、示例定义和逐条行为断言混在同一文件，测试矩阵难以维护（CMakeLists.txt:93）。 Fixed in commit cfc7f75 · 根 CMakeLists 2438→99 行：86 个示例 target 定义抽到 cmake/examples.cmake（401 行），569 条 add_test + set_tests_properties 行为断言抽到 cmake/tests.cmake（2596 行），根文件在 if(CLASP_BUILD_EXAMPLES) 内先 include examples 再 include tests · configure + 569 测试注册顺序验证 · 569/569 CTest green
- [x] LAY-004 [P2] 拆分 tests/completion_coverage.cpp（1464 行）——补全协议、shell 转义和端到端树断言集中，可按 Bash/Zsh/Fish/PowerShell 分域（tests/completion_coverage.cpp:1）。 Fixed in commit b423c6e · 按关注域拆为共享 tests/completion_support.hpp 的四个 TU：completion_protocol.cpp（命令树解析/协议断言）、completion_scripts.cpp（shell 脚本生成+转义）、completion_e2e.cpp（真实 bash/zsh/fish 执行），main 留在 completion_coverage.cpp；completion_coverage_test 目标编译四源 · 569/569 CTest green
- [x] LAY-005 [P2] 拆分 tests/branch_parser.cpp（1129 行）——命令分支解析、建议、遍历与错误输出覆盖集中，适合按执行解析阶段拆分（tests/branch_parser.cpp:1）。 Fixed in commit 3195cee · 按执行解析阶段拆为共享 tests/branch_support.hpp 的四个 TU：branch_parse_paths.cpp（flag token 解码路径）、branch_value_types.cpp（类型校验：窄整型/bytes 单位/IP/CIDR/URL/注解）、branch_access_lookup.cpp（getFlag 解析链/外部源/查找/建议），main 留在 branch_parser.cpp；branch_parser_test 目标编译四源 · 569/569 CTest green

### c) 目录/分类问题

- `src/command.cpp` 只含注释却参与库编译，而 `Command` 实际定义在头文件；目录布局暗示实现分离，但当前没有对应实现单元（src/command.cpp:1、CMakeLists.txt:39）。
- `packaging/vcpkg/portfile.cmake` 仍引用 `v0.1.0` 且 `SHA512 0`，与当前 `CMakeLists.txt` 和伞形头的 `0.1.2` 版本契约不一致（packaging/vcpkg/portfile.cmake:4、packaging/vcpkg/portfile.cmake:5、CMakeLists.txt:2、include/clasp/clasp.hpp:9）。
- 根 `CMakeLists.txt` 在主构建脚本中内联大量示例 target、CTest 名称和精确输出断言，测试契约与库构建配置没有目录/文件边界（CMakeLists.txt:93、CMakeLists.txt:2340）。
