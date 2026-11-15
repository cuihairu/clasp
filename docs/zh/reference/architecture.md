---
title: 源码架构
---

# 源码架构

- [术语表](./glossary.md)
- [English architecture](/reference/architecture)

本文依据当前源码归纳职责、状态、配置、生命周期和调用面；分层表达的是职责边界，不是目录名。

## 1. 契约、构建与打包

构建契约固定 C++17，校验项目版本与 `CLASP_VERSION_*` 宏一致，编译少量非头文件实现，并导出 `clasp::clasp` CMake 包（`CMakeLists.txt:2`、`CMakeLists.txt:4`、`CMakeLists.txt:11`、`CMakeLists.txt:39`、`CMakeLists.txt:52`、`CMakeLists.txt:61`、`CMakeLists.txt:77`）。配置输入是 `CLASP_BUILD_EXAMPLES`、`CLASP_ENABLE_COVERAGE`、标准安装目录和共享库属性（`CMakeLists.txt:23`、`CMakeLists.txt:27`、`CMakeLists.txt:54`、`CMakeLists.txt:63`）。公开头文件树和 package config 是消费方接口（`CMakeLists.txt:46`、`CMakeLists.txt:70`、`cmake/claspConfig.cmake.in:3`）。

该层保存构建元数据和安装产物，不是运行态。示例与测试是可选消费契约：启用后构建可执行文件、注册 CTest，并锁定可见字符串与解析行为（`CMakeLists.txt:93`、`cmake/tests.cmake:5`、`cmake/examples.cmake:401`、`cmake/examples.cmake:406`）。vcpkg 目录是另一个打包入口草案，固定在 `v0.1.2` 且 `SHA512` 已填（`packaging/vcpkg/portfile.cmake:1`、`packaging/vcpkg/portfile.cmake:4`、`packaging/vcpkg/README.md:3`）。

## 2. 命令树与应用模型

`Command` 是进程内应用模型，拥有名称、别名、元数据、分组、本地/持久标志、子节点所有权、输出流、回调、覆盖项、上下文以及表现层/补全设置（`include/clasp/command.hpp:71`、`include/clasp/command.hpp:220`、`include/clasp/command.hpp:92`、`include/clasp/command.hpp:3199`）。`Flag` 是声明式标志元数据，包含长短名、描述、类型化默认值、required/hidden/deprecated 状态、注解和无值默认（`include/clasp/command.hpp:71` 处建模，字段见 `include/clasp/flag.hpp:23`、`include/clasp/flag.hpp:61`）。兼容头只转发到规范头文件（`include/clasp/command.h:4`）。

命令树的状态是进程内内存。`addCommand` 把值转移成唯一拥有的子节点并立即改写父指针（`include/clasp/command.hpp:220`）；手写移动构造/赋值保证所有权转移后子节点不会指向旧对象（`include/clasp/command.hpp:83`、`include/clasp/command.hpp:87`）。配置通过 builder 方法输入：别名与注解（`include/clasp/command.hpp:203`）、标志与类型助手（`include/clasp/command.hpp:238`、`include/clasp/command.hpp:251`）、钩子与动作（`include/clasp/command.hpp:54`）、标志组约束（`include/clasp/command.hpp:966`）、模板与输出流（`include/clasp/command.hpp:1165`、`include/clasp/command.hpp:92`）、上下文（`include/clasp/command.hpp:1218`）。

## 3. 执行编排

`run`、`execute`、命令解析、校验和钩子/动作编排构成运行边界。`execute()` 可用先前注入的参数合成 `argv`，随后与 CLI 入口一样进入 `run`（`include/clasp/command.hpp:1256`、`include/clasp/command.hpp:1270`）。`resolveForExecution` 遍历原始 token，解析别名/子命令，并记录是否要执行 help/version（`include/clasp/command.hpp:2954`、`include/clasp/command.hpp:3101`）。

解析目标确定后，`run` 使用该命令的有效标志构造 `Parser`，把继承覆盖项转换成解析选项，将解析和颜色错误交给错误格式化器，再依次处理 help/version、外部源和值绑定（`include/clasp/command.hpp:1287`、`include/clasp/command.hpp:1310`、`include/clasp/command.hpp:1326`、`include/clasp/command.hpp:1359`）。接着校验位置参数、必需标志、标志组和弃用状态，最后调用目标命令（`include/clasp/command.hpp:1352`、`include/clasp/command.hpp:1376`、`include/clasp/command.hpp:1382`、`include/clasp/command.hpp:1402`）。钩子沿父链执行持久前置、本地前置、任一动作形态，再按相反归属顺序执行后置钩子（`include/clasp/command.hpp:2482`）。主返回通道是整数进程退出码；动作可选择返回整数或错误消息（`include/clasp/command.hpp:54`）。

## 4. Token 解析、值与校验

`Parser` 拥有一次调用的解码结果。`Options` 控制未知标志、短旗组合、布尔取反、建议、原始模式和键规范化（`include/clasp/parser.hpp:31`）。构造时注册内建项和声明别名/类型/默认值，随后把 `--`、`--key=value`、短旗组、短旗内联值和分离值记录为有序 occurrence（`include/clasp/parser.hpp:45`、`include/clasp/parser.hpp:759`、`include/clasp/parser.hpp:860`）。未知标志经共享工具生成编辑距离建议（`include/clasp/parser.hpp:940`、`include/clasp/utils.hpp:13`、`include/clasp/utils.hpp:33`）。

运行态是内存映射：显式值、外部 scalar/multi 值、别名、类型、默认值、无值默认、扩展键和 positionals（`include/clasp/parser.hpp:950`）。访问器明确优先级：显式 CLI 值优先，其次外部 multi/scalar，最后声明默认（`include/clasp/parser.hpp:223`）。自定义 `Value` 回调刻意在外部源合并后应用；`Command::applyBoundFlagValues` 读取显式值并按 occurrence 顺序调用 `Value::set`（`include/clasp/value.hpp:13`、`include/clasp/command.hpp:3491`）。

类型校验先由默认值 variant 得到基础 Kind，再用 truthy 注解特化字符串键（`include/clasp/parser.hpp:625`、`include/clasp/parser.hpp:759`）。`normalizeValue` 校验并规范化布尔、多种宽度整数、浮点、时长、IP/mask/CIDR、URL、字节容量、IP 列表和 Base64（`include/clasp/parser.hpp:629`）。基础数值/时长解析隔离在 `detail/value_parse.hpp`，严格 Base64 编解码也在该处（`include/clasp/detail/value_parse.hpp:25`、`include/clasp/detail/value_parse.hpp:188`）。`Flag::convertToFlagValue` 把同一批基础解析适配到公开 `FlagValue`（`src/flag.cpp:8`、`include/clasp/flag.hpp:13`）。

## 5. 外部源集成

外部源层把环境变量和文件值转换成已校验的 parser 状态。环境绑定是命令树元数据，子命令覆盖父命令（`include/clasp/command.hpp:188`、`include/clasp/command.hpp:3481`）。配置路径可固定，也可来自规范化后的标志（`include/clasp/command.hpp:193`、`include/clasp/command.hpp:198`）。

`applyExternalSources` 读取 env-like 标量格式，并用头内小型解析器处理 JSON、YAML、TOML、INI/CFG；JSON 有嵌套深度保护（`detail/config_parse.hpp:48`、`detail/config_parse.hpp:94`、`include/clasp/command.hpp:3536`）。它把键扁平化成长标志别名，包括裸键、下划线、点号和 `varName` 形式（`include/clasp/command.hpp:3571`），再写入已校验 multi 与 scalar 值（`include/clasp/command.hpp:3613`）。进程环境覆盖配置文件；环境访问就是平台边界（`include/clasp/command.hpp:3603`、`include/clasp/color.hpp:46`）。因此文档化优先级为 CLI、环境、配置文件、默认值（`docs/guide/config.md:3`）。状态只在运行期读取文件内容和进程环境；没有守护进程、数据库或网络源。

## 6. 表现层、导出与终端适配

表现层把命令/标志状态转换成 help、usage、version、Markdown 和 roff。help 渲染收集可见命令与本地/全局标志，支持函数覆盖或受限占位符模板，而不是等价 Go template（`include/clasp/command.hpp:1409`、`include/clasp/command.hpp:2356`、`include/clasp/command.hpp:2245`）。usage 走同类覆盖链（`include/clasp/command.hpp:1831`）；Markdown 与 manpage 是独立文本协议（`include/clasp/command.hpp:1460`、`include/clasp/command.hpp:1499`）。输出汇可继承也可局部覆盖（`include/clasp/command.hpp:1862`）。

颜色适配器封装终端能力、ANSI 序列、主题、`NO_COLOR`、`TERM=dumb` 和 Windows VT 模式（`include/clasp/color.hpp:66`、`include/clasp/color.hpp:78`、`include/clasp/color.hpp:88`、`include/clasp/color.hpp:95`、`src/color.cpp:14`、`src/color.cpp:32`）。表现层根据颜色 owner 的运行模式和适配器判断是否着色（`include/clasp/command.hpp:2065`、`include/clasp/command.hpp:2079`）。颜色模式/主题也可来自持久 CLI 标志：原始参数只预扫描到子命令边界，正常解析后再应用 parser 值（`include/clasp/command.hpp:106`、`include/clasp/command.hpp:2112`、`include/clasp/command.hpp:2136`）。调用通道是文本：`stdout`、`stderr`、调用方流、ANSI、Markdown 和 roff。

## 7. 补全协议与 Shell 生成

补全有两个协作面。命令树组装时，`enableCompletion` 按 `CompletionConfig` 安装脚本生成命令和隐藏动态命令（`include/clasp/command.hpp:1541`、`include/clasp/command.hpp:3891`）。生成脚本通过 shell 的普通进程调用通道回调这些隐藏命令（`include/clasp/command.hpp:3912`、`include/clasp/command.hpp:3937`）。

动态路径把 shell words 当作 `argv`，解析将要执行的命令，查询 traversal 感知的标志元数据和回调，并输出值/描述（`include/clasp/command.hpp:2562`、`include/clasp/command.hpp:2601`、`include/clasp/command.hpp:2725`）。指令是 Cobra 兼容的数字位掩码（`include/clasp/command.hpp:39`）；文件名/目录助手把扩展名或目录转换为产生指令的回调（`include/clasp/command.hpp:885`、`include/clasp/command.hpp:904`）。静态生成覆盖 Bash/Zsh 共用生成器、Fish 和 PowerShell，并带 shell 专属转义助手（`include/clasp/command.hpp:1533`、`detail/shell_escape.hpp:132`、`include/clasp/command.hpp:3622`、`include/clasp/command.hpp:3777`）。协议脚本运行期生成，仓库没有单独的静态协议文件。

## 8. 消费验证与文档契约

示例是小规模可执行演示，覆盖模型、执行、类型化值、补全、外部源和文档导出（`examples/basic_example.cpp:7`、`examples/completion_example.cpp:6`、`examples/config_json_example.cpp:7`、`examples/docs_example.cpp:7`）。单测/集成测试直接调用 `Command` 与 `Parser`，注入流，并运行最终 CLI，而非探测私有内部（`tests/completion.cpp:13`、`tests/config_parse_coverage.cpp:64`、`tests/execute.cpp:7`）。CMake 把示例变成黑盒 CTest，并锁定 Cobra 兼容消息（`cmake/examples.cmake:6`、`cmake/examples.cmake:401`）。

文档站是该契约的人工可读投影。英文与中文指南分别描述命令、标志、补全、配置优先级、模板和自定义 `Value`（`docs/guide/index.md:3`、`docs/zh/guide/index.md:3`）；VitePress 配置双语言并排除原始审阅档案（`docs/.vitepress/config.ts:5`、`docs/.vitepress/config.ts:15`、`docs/.vitepress/config.ts:8`）。跨语言内容按语义同步；本页和术语表补充源码推导出的抽象模型。

## 拆分理由

1. **区分声明与一次调用的解码结果。** `Flag` 是命令保留的元数据（`include/clasp/flag.hpp:61`），`Parser` 单独记录有序运行 occurrence（`include/clasp/parser.hpp:950`）。混在一起会模糊持久树所有权和单次运行状态。
2. **把命令所有权和父指针不变式放在一处。** 移动 `Command` 必须重写所有子节点父指针；手写移动和 `reparent` 正是为避免普通成员移动留下悬挂父指针（`include/clasp/command.hpp:83`、`include/clasp/command.hpp:220`、`include/clasp/command.hpp:2557`）。
3. **先解析执行节点，再构造 parser。** `resolveForExecution` 消耗命令 token 并生成命令专用 `argv`（`include/clasp/command.hpp:2954`、`include/clasp/command.hpp:2974`），`Parser` 再只解析该节点有效标志（`include/clasp/command.hpp:1310`、`include/clasp/command.hpp:1319`）。继承语义与命令本地 positional 因此互不纠缠。
4. **让源优先级在访问器边界可见。** `getFlag` 明确按 CLI、外部 multi、外部 scalar、默认值查询（`include/clasp/parser.hpp:223`），文档也暴露同一优先级（`docs/guide/config.md:3`）。隐藏合并层会让覆盖行为难以推理。
5. **自定义 `Value` 回调在所有源合并后触发。** 契约要求 `set()` 按 occurrence 顺序收到最终有效值（`include/clasp/value.hpp:13`）；执行流程也确实先应用外部源再应用绑定（`include/clasp/command.hpp:1359`、`include/clasp/command.hpp:3491`）。提前回调会破坏感知配置/环境的自定义类型。
6. **生成脚本和动态补全属于一个协议。** 所有 shell 最终调用配置的 `__complete`/`__completeNoDesc`（`include/clasp/command.hpp:3891`、`include/clasp/command.hpp:3912`、`include/clasp/command.hpp:3937`），`completeWords` 统一解析上下文（`include/clasp/command.hpp:2725`）。Shell 生成器只需变化语法和转义。
7. **隔离平台与文本格式细节。** 终端能力收敛在 `color`（`src/color.cpp:14`、`src/color.cpp:32`）；数值/Base64 基元收敛在 `detail/value_parse.hpp`（`include/clasp/detail/value_parse.hpp:25`、`include/clasp/detail/value_parse.hpp:188`）；文件语法解析收敛在 `applyExternalSources`（`include/clasp/command.hpp:3519`、`include/clasp/command.hpp:3536`）。命令编排不必内联每条平台规则。
8. **用可执行消费契约固定外部可见文本。** CMake 构建示例并断言 Cobra 兼容 help/error 字符串（`cmake/examples.cmake:401`、`cmake/tests.cmake:2495`、`cmake/tests.cmake:2505`），测试再覆盖进程内行为（`tests/execute.cpp:7`、`tests/completion.cpp:13`）。兼容性关注点因此与实现组织分离。
