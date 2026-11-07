---
title: Architecture Glossary
---

# Architecture Glossary

- [Architecture](./architecture.md)
- [中文术语表](/zh/reference/glossary)

| 术语 | English | 定义 | Evidence |
| --- | --- | --- | --- |
| Clasp | Clasp | C++17 command-line library whose public umbrella exposes command, flag, value, and color APIs. | `include/clasp/clasp.hpp:3`, `CMakeLists.txt:4` |
| 命令树 | Command tree | A root-owned tree of uniquely owned `Command` nodes linked through parent pointers for lookup, inheritance, and path rendering. | `include/clasp/command.hpp:354`, `include/clasp/command.hpp:3198` |
| 命令 | Command | An application node with names/aliases, descriptions, flags, callbacks, child ownership, output sinks, and inherited overrides. | `include/clasp/command.hpp:69`, `include/clasp/command.hpp:3189` |
| 本地标志 | Local flag | A flag stored on one command and included in that command's parse surface. | `include/clasp/command.hpp:372`, `include/clasp/command.hpp:3196` |
| 持久标志 | Persistent flag | A command-owned flag inherited downward for parsing and shown to descendants as a global flag. | `include/clasp/command.hpp:962`, `include/clasp/command.hpp:2443` |
| 有效标志集 | Effective flag set | The resolved parse surface formed by a command's own flags plus inherited persistent flags. | `include/clasp/command.hpp:1396`, `include/clasp/command.hpp:2175` |
| 标志 | Flag | Declarative metadata containing long/short names, help text, typed default, constraints, annotations, and optional bare-value default. | `include/clasp/flag.hpp:23`, `include/clasp/flag.hpp:61` |
| 标志值 | Flag value | The public variant of boolean, signed/unsigned integer, floating-point, duration, or string defaults. | `include/clasp/flag.hpp:13` |
| 标志注解 | Flag annotation | A string key/value marker that extends a string or numeric flag with specialized validation and canonicalization. | `include/clasp/flag.hpp:54`, `include/clasp/parser.hpp:1125` |
| 自定义值 | Custom value | The `Value` extension interface used to parse one occurrence and describe/display a flag type. | `include/clasp/value.hpp:16`, `include/clasp/value.hpp:21` |
| 值绑定 | Value binding | A normalized flag-name to live `Value*` mapping used after parsing and source merging. | `include/clasp/command.hpp:1123`, `include/clasp/command.hpp:3246` |
| 解析器选项 | Parser options | Per-invocation toggles for unknown flags, grouping, negation, suggestions, raw parsing, and key normalization. | `include/clasp/parser.hpp:30`, `include/clasp/command.hpp:1397` |
| 规范标志键 | Canonical flag key | The normalized name under which aliases, kinds, defaults, extension keys, and occurrences are indexed. | `include/clasp/parser.hpp:592`, `include/clasp/parser.hpp:1450` |
| 标志出现 | Flag occurrence | One recorded value in the parser's ordered per-flag value list; repeated flags preserve order. | `include/clasp/parser.hpp:1348`, `include/clasp/parser.hpp:263` |
| 显式值 | Explicit value | A CLI or external-source value, deliberately distinguished from a declared default. | `include/clasp/parser.hpp:270`, `include/clasp/parser.hpp:285` |
| 外部源 | External source | Environment variables and supported config files injected as scalar or multi values before bound callbacks run. | `include/clasp/command.hpp:3673`, `include/clasp/command.hpp:4282` |
| 环境绑定 | Environment binding | A mapping from normalized flag name to process environment variable name, merged child-over-parent. | `include/clasp/command.hpp:322`, `include/clasp/command.hpp:3635` |
| 配置路径 | Config path | A fixed config location or a value obtained from a designated normalized flag, read once per execution. | `include/clasp/command.hpp:327`, `include/clasp/command.hpp:4201` |
| 值规范化 | Value normalization | Validation plus canonical rewriting for booleans, widths, durations, IP forms, URLs, byte sizes, CSV IP slices, and Base64. | `include/clasp/parser.hpp:1125`, `include/clasp/detail/value_parse.hpp:25` |
| 执行解析 | Execution resolution | Raw `argv` traversal that maps aliases/subcommands to a target command and command-scoped argument vector. | `include/clasp/command.hpp:3034`, `include/clasp/command.hpp:3036` |
| 生命周期钩子 | Lifecycle hook | Pre/post callbacks, with persistent variants inherited through the parent chain around a command action. | `include/clasp/command.hpp:58`, `include/clasp/command.hpp:2555` |
| 动作 | Action | The resolved command's executable callback, supplied as an integer-returning or error-returning function. | `include/clasp/command.hpp:52`, `include/clasp/command.hpp:1902` |
| 参数校验器 | Args validator | A positional-argument predicate used after parser-level validation and before required/group checks. | `include/clasp/command.hpp:57`, `include/clasp/command.hpp:1461` |
| 标志组 | Flag group | A declared mutually-exclusive, one-required, or required-together relationship evaluated against parsed flags. | `include/clasp/command.hpp:1070`, `include/clasp/command.hpp:2495` |
| 输出汇 | Output sink | Caller-selected or inherited `stdout`/`stderr` streams used by rendering, actions, and errors. | `include/clasp/command.hpp:232`, `include/clasp/command.hpp:1935` |
| 颜色主题 | Color theme | A named set of ANSI role codes selected at runtime from built-in static themes. | `include/clasp/color.hpp:95`, `include/clasp/command.hpp:2167` |
| 终端适配器 | Terminal adapter | The platform boundary that checks TTY state, reads color-related environment, and enables Windows VT processing. | `src/color.cpp:14`, `src/color.cpp:32` |
| 帮助模板 | Help template | A restricted placeholder template or complete callback controlling help text composition. | `include/clasp/command.hpp:1263`, `include/clasp/command.hpp:1494` |
| 用法行 | Usage line | Generated one-line synopsis showing command path, command slot, and optional flags slot. | `include/clasp/command.hpp:1971` |
| 文档导出 | Documentation export | Rendering of command state as recursive Markdown or a roff manpage independent of terminal help. | `include/clasp/command.hpp:1545`, `include/clasp/command.hpp:1584` |
| 补全配置 | Completion configuration | Root-level names/toggles controlling whether completion and dynamic-complete commands are installed. | `include/clasp/command.hpp:1626`, `include/clasp/command.hpp:4560` |
| 补全指令 | Completion directive | A Cobra-compatible numeric bitmask conveyed as a `:<number>` token to shell consumers. | `include/clasp/command.hpp:37`, `include/clasp/command.hpp:1007` |
| 动态补全 | Dynamic completion | Runtime resolution of shell words into command candidates, flags, custom values, and descriptions. | `include/clasp/command.hpp:2723`, `include/clasp/command.hpp:2815` |
| 生成补全脚本 | Generated completion script | Emitted Bash/Zsh, Fish, or PowerShell code that later invokes the program's hidden complete commands. | `include/clasp/command.hpp:1618`, `include/clasp/command.hpp:4581` |
| 消费者契约 | Consumer contract | Opt-in examples, unit tests, and black-box CTest assertions that pin APIs and visible behavior. | `CMakeLists.txt:93`, `CMakeLists.txt:2340` |
| 构建包 | Build package | The CMake target/install/export surface consumed as `clasp::clasp`, with version consistency enforced at configure time. | `CMakeLists.txt:11`, `CMakeLists.txt:61` |
