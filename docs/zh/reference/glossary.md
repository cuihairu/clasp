---
title: 架构术语表
---

# 架构术语表

- [架构解说](./architecture.md)
- [English glossary](/reference/glossary)

| 术语 | English | 定义 | 证据 |
| --- | --- | --- | --- |
| Clasp | Clasp | C++17 命令行库；公开伞形头暴露 command、flag、value 和 color API。 | `include/clasp/clasp.hpp:3`、`CMakeLists.txt:4` |
| 命令树 | Command tree | 根节点唯一拥有子 `Command`，通过父指针完成查找、继承和路径渲染。 | `include/clasp/command.hpp:220`、`include/clasp/command.hpp:3208` |
| 命令 | Command | 应用节点，包含名称/别名、描述、标志、回调、子节点所有权、输出汇和可继承覆盖项。 | `include/clasp/command.hpp:71`、`include/clasp/command.hpp:3199` |
| 本地标志 | Local flag | 保存在单个命令上的标志，进入该命令解析面。 | `include/clasp/command.hpp:238`、`include/clasp/command.hpp:3206` |
| 持久标志 | Persistent flag | 命令拥有并向下继承的标志；对后代表现为 global flag。 | `include/clasp/command.hpp:828`、`include/clasp/command.hpp:2370` |
| 有效标志集 | Effective flag set | 当前命令自有标志加继承持久标志后形成的解析面。 | `include/clasp/command.hpp:1310`、`include/clasp/command.hpp:1608` |
| 标志 | Flag | 声明式元数据，含长短名、帮助文本、类型化默认值、约束、注解和无值默认。 | `include/clasp/flag.hpp:23`、`include/clasp/flag.hpp:61` |
| 标志值 | Flag value | 布尔、有/无符号整数、浮点、时长或字符串组成的公开默认值 variant。 | `include/clasp/flag.hpp:13` |
| 标志注解 | Flag annotation | 字符串键值标记，让 string/numeric flag 获得专门校验与规范化。 | `include/clasp/flag.hpp:54`、`include/clasp/parser.hpp:629` |
| 自定义值 | Custom value | `Value` 扩展接口，用于解析单个 occurrence 并描述/显示 flag 类型。 | `include/clasp/value.hpp:16`、`include/clasp/value.hpp:21` |
| 值绑定 | Value binding | 规范化标志名到存活 `Value*` 的映射，在解析和源合并后使用。 | `include/clasp/command.hpp:1023`、`include/clasp/command.hpp:3259` |
| 解析器选项 | Parser options | 单次调用的未知标志、分组、取反、建议、原始解析和键规范化开关。 | `include/clasp/parser.hpp:31`、`include/clasp/command.hpp:1311` |
| 规范标志键 | Canonical flag key | 别名、类型、默认值、扩展键和 occurrence 统一索引的规范化名称。 | `include/clasp/parser.hpp:609`、`include/clasp/parser.hpp:953` |
| 标志出现 | Flag occurrence | parser 每个 flag 的有序值列表中的一条记录；重复值保留顺序。 | `include/clasp/parser.hpp:856`、`include/clasp/parser.hpp:276` |
| 显式值 | Explicit value | CLI 或外部源提供的值，刻意与声明默认值区分。 | `include/clasp/parser.hpp:283`、`include/clasp/parser.hpp:298` |
| 外部源 | External source | 在绑定回调前注入的 environment 和受支持 config 文件 scalar/multi 值。 | `include/clasp/command.hpp:3519`、`include/clasp/command.hpp:3613` |
| 环境绑定 | Environment binding | 规范化 flag 名到进程环境变量名的映射，子命令覆盖父命令。 | `include/clasp/command.hpp:188`、`include/clasp/command.hpp:3481` |
| 配置路径 | Config path | 固定配置位置或来自指定规范化 flag 的位置；每次执行读取一次。 | `include/clasp/command.hpp:193`、`include/clasp/command.hpp:3532` |
| 值规范化 | Value normalization | 布尔、多种宽度、时长、IP 形态、URL、字节容量、CSV IP 列表和 Base64 的校验加规范重写。 | `include/clasp/parser.hpp:629`、`include/clasp/detail/value_parse.hpp:25` |
| 执行解析 | Execution resolution | 遍历原始 `argv`，把别名/子命令映射为目标命令和命令域参数向量。 | `include/clasp/command.hpp:2954`、`include/clasp/command.hpp:2956` |
| 生命周期钩子 | Lifecycle hook | 动作前后的 pre/post 回调；persistent 变体沿父链继承。 | `include/clasp/command.hpp:60`、`include/clasp/command.hpp:2482` |
| 动作 | Action | 目标命令的可执行回调，可选择返回整数或错误消息。 | `include/clasp/command.hpp:54`、`include/clasp/command.hpp:1829` |
| 参数校验器 | Args validator | 在 parser 级校验后、required/group 检查前使用的位置参数谓词。 | `include/clasp/command.hpp:59`、`include/clasp/command.hpp:1376` |
| 标志组 | Flag group | 对已解析 flags 评估的互斥、至少一个必填或同时必填关系。 | `include/clasp/command.hpp:966`、`include/clasp/command.hpp:2422` |
| 输出汇 | Output sink | 渲染、动作和错误使用的调用方选择或继承的 `stdout`/`stderr` 流。 | `include/clasp/command.hpp:92`、`include/clasp/command.hpp:1862` |
| 颜色主题 | Color theme | 运行期从内建静态主题中选择的 ANSI role code 集合。 | `include/clasp/color.hpp:95`、`include/clasp/command.hpp:2094` |
| 终端适配器 | Terminal adapter | 检查 TTY、读取颜色环境并启用 Windows VT 的平台边界。 | `src/color.cpp:14`、`src/color.cpp:32` |
| 帮助模板 | Help template | 控制帮助文本组成的受限占位符模板或完整回调。 | `include/clasp/command.hpp:1165`、`include/clasp/command.hpp:1409` |
| 用法行 | Usage line | 生成的单行 synopsis，包含命令路径、命令槽和可选 flags 槽。 | `include/clasp/command.hpp:1898` |
| 文档导出 | Documentation export | 将命令状态渲染成递归 Markdown 或 roff manpage，独立于终端 help。 | `include/clasp/command.hpp:1460`、`include/clasp/command.hpp:1499` |
| 补全配置 | Completion configuration | 根级名称/开关，控制是否安装 completion 和动态 complete 命令。 | `include/clasp/command.hpp:1541`、`include/clasp/command.hpp:3891` |
| 补全指令 | Completion directive | 以 `:<number>` token 传给 shell 消费方的 Cobra 兼容数字位掩码。 | `include/clasp/command.hpp:39`、`include/clasp/command.hpp:896` |
| 动态补全 | Dynamic completion | 运行期把 shell words 解析成命令候选、flags、自定义值和描述。 | `include/clasp/command.hpp:2601`、`include/clasp/command.hpp:2725` |
| 生成补全脚本 | Generated completion script | 输出的 Bash/Zsh、Fish 或 PowerShell 代码，之后回调程序隐藏 complete 命令。 | `include/clasp/command.hpp:1533`、`include/clasp/command.hpp:3912` |
| 消费者契约 | Consumer contract | 可选 examples、单测和黑盒 CTest，共同固定 API 与可见行为。 | `CMakeLists.txt:91`、`cmake/examples.cmake:401` |
| 构建包 | Build package | 消费方使用的 CMake target/install/export 面 `clasp::clasp`，配置期强制版本一致。 | `CMakeLists.txt:11`、`CMakeLists.txt:59` |
