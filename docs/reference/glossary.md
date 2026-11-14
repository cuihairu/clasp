---
title: Architecture Glossary
---

# Architecture Glossary

- [Architecture](./architecture.md)
- [中文术语表](/zh/reference/glossary)

| 术语 | English | 定义 | Evidence |
| --- | --- | --- | --- |
| Clasp | Clasp | C++17 command-line library whose public umbrella exposes command, flag, value, and color APIs. | `include/clasp/clasp.hpp:3`, `CMakeLists.txt:4` |
| 命令树 | Command tree | A root-owned tree of uniquely owned `Command` nodes linked through parent pointers for lookup, inheritance, and path rendering. | `include/clasp/command.hpp:220`, `include/clasp/command.hpp:3208` |
| 命令 | Command | An application node with names/aliases, descriptions, flags, callbacks, child ownership, output sinks, and inherited overrides. | `include/clasp/command.hpp:71`, `include/clasp/command.hpp:3199` |
| 本地标志 | Local flag | A flag stored on one command and included in that command's parse surface. | `include/clasp/command.hpp:238`, `include/clasp/command.hpp:3206` |
| 持久标志 | Persistent flag | A command-owned flag inherited downward for parsing and shown to descendants as a global flag. | `include/clasp/command.hpp:828`, `include/clasp/command.hpp:2370` |
| 有效标志集 | Effective flag set | The resolved parse surface formed by a command's own flags plus inherited persistent flags. | `include/clasp/command.hpp:1310`, `include/clasp/command.hpp:1608` |
| 标志 | Flag | Declarative metadata containing long/short names, help text, typed default, constraints, annotations, and optional bare-value default. | `include/clasp/flag.hpp:23`, `include/clasp/flag.hpp:61` |
| 标志值 | Flag value | The public variant of boolean, signed/unsigned integer, floating-point, duration, or string defaults. | `include/clasp/flag.hpp:13` |
| 标志注解 | Flag annotation | A string key/value marker that extends a string or numeric flag with specialized validation and canonicalization. | `include/clasp/flag.hpp:54`, `include/clasp/parser.hpp:629` |
| 自定义值 | Custom value | The `Value` extension interface used to parse one occurrence and describe/display a flag type. | `include/clasp/value.hpp:16`, `include/clasp/value.hpp:21` |
| 值绑定 | Value binding | A normalized flag-name to live `Value*` mapping used after parsing and source merging. | `include/clasp/command.hpp:1023`, `include/clasp/command.hpp:3259` |
| 解析器选项 | Parser options | Per-invocation toggles for unknown flags, grouping, negation, suggestions, raw parsing, and key normalization. | `include/clasp/parser.hpp:31`, `include/clasp/command.hpp:1311` |
| 规范标志键 | Canonical flag key | The normalized name under which aliases, kinds, defaults, extension keys, and occurrences are indexed. | `include/clasp/parser.hpp:609`, `include/clasp/parser.hpp:953` |
| 标志出现 | Flag occurrence | One recorded value in the parser's ordered per-flag value list; repeated flags preserve order. | `include/clasp/parser.hpp:856`, `include/clasp/parser.hpp:276` |
| 显式值 | Explicit value | A CLI or external-source value, deliberately distinguished from a declared default. | `include/clasp/parser.hpp:283`, `include/clasp/parser.hpp:298` |
| 外部源 | External source | Environment variables and supported config files injected as scalar or multi values before bound callbacks run. | `include/clasp/command.hpp:3519`, `include/clasp/command.hpp:3613` |
| 环境绑定 | Environment binding | A mapping from normalized flag name to process environment variable name, merged child-over-parent. | `include/clasp/command.hpp:188`, `include/clasp/command.hpp:3481` |
| 配置路径 | Config path | A fixed config location or a value obtained from a designated normalized flag, read once per execution. | `include/clasp/command.hpp:193`, `include/clasp/command.hpp:3532` |
| 值规范化 | Value normalization | Validation plus canonical rewriting for booleans, widths, durations, IP forms, URLs, byte sizes, CSV IP slices, and Base64. | `include/clasp/parser.hpp:629`, `include/clasp/detail/value_parse.hpp:25` |
| 执行解析 | Execution resolution | Raw `argv` traversal that maps aliases/subcommands to a target command and command-scoped argument vector. | `include/clasp/command.hpp:2954`, `include/clasp/command.hpp:2956` |
| 生命周期钩子 | Lifecycle hook | Pre/post callbacks, with persistent variants inherited through the parent chain around a command action. | `include/clasp/command.hpp:60`, `include/clasp/command.hpp:2482` |
| 动作 | Action | The resolved command's executable callback, supplied as an integer-returning or error-returning function. | `include/clasp/command.hpp:54`, `include/clasp/command.hpp:1829` |
| 参数校验器 | Args validator | A positional-argument predicate used after parser-level validation and before required/group checks. | `include/clasp/command.hpp:59`, `include/clasp/command.hpp:1376` |
| 标志组 | Flag group | A declared mutually-exclusive, one-required, or required-together relationship evaluated against parsed flags. | `include/clasp/command.hpp:966`, `include/clasp/command.hpp:2422` |
| 输出汇 | Output sink | Caller-selected or inherited `stdout`/`stderr` streams used by rendering, actions, and errors. | `include/clasp/command.hpp:92`, `include/clasp/command.hpp:1862` |
| 颜色主题 | Color theme | A named set of ANSI role codes selected at runtime from built-in static themes. | `include/clasp/color.hpp:95`, `include/clasp/command.hpp:2094` |
| 终端适配器 | Terminal adapter | The platform boundary that checks TTY state, reads color-related environment, and enables Windows VT processing. | `src/color.cpp:14`, `src/color.cpp:32` |
| 帮助模板 | Help template | A restricted placeholder template or complete callback controlling help text composition. | `include/clasp/command.hpp:1165`, `include/clasp/command.hpp:1409` |
| 用法行 | Usage line | Generated one-line synopsis showing command path, command slot, and optional flags slot. | `include/clasp/command.hpp:1898` |
| 文档导出 | Documentation export | Rendering of command state as recursive Markdown or a roff manpage independent of terminal help. | `include/clasp/command.hpp:1460`, `include/clasp/command.hpp:1499` |
| 补全配置 | Completion configuration | Root-level names/toggles controlling whether completion and dynamic-complete commands are installed. | `include/clasp/command.hpp:1541`, `include/clasp/command.hpp:3891` |
| 补全指令 | Completion directive | A Cobra-compatible numeric bitmask conveyed as a `:<number>` token to shell consumers. | `include/clasp/command.hpp:39`, `include/clasp/command.hpp:896` |
| 动态补全 | Dynamic completion | Runtime resolution of shell words into command candidates, flags, custom values, and descriptions. | `include/clasp/command.hpp:2601`, `include/clasp/command.hpp:2725` |
| 生成补全脚本 | Generated completion script | Emitted Bash/Zsh, Fish, or PowerShell code that later invokes the program's hidden complete commands. | `include/clasp/command.hpp:1533`, `include/clasp/command.hpp:3912` |
| 消费者契约 | Consumer contract | Opt-in examples, unit tests, and black-box CTest assertions that pin APIs and visible behavior. | `CMakeLists.txt:93`, `cmake/examples.cmake:401` |
| 构建包 | Build package | The CMake target/install/export surface consumed as `clasp::clasp`, with version consistency enforced at configure time. | `CMakeLists.txt:11`, `CMakeLists.txt:61` |
