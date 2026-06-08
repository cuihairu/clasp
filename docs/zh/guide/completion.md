# Completion（补全）

## 生成脚本

Clasp 支持 bash/zsh/fish/powershell completion 脚本生成（对应 `examples/completion_example.cpp`）。

在根命令上调用 `enableCompletion()` 会安装：

- 面向用户的 `completion <shell>` 命令
- 供生成脚本调用的内部 `__complete` 和 `__completeNoDesc` 命令

也可以通过 `enableCompletion(Command::CompletionConfig{...})` 重命名或关闭这些命令。

## 动态补全（__complete）

- positional：`validArgs()` / `validArgsFunction()`
- flag value：`registerFlagCompletion("--flag", func)`

动态补全会结合命令遍历和子命令上下文，按实际将要执行的命令解析候选项。

## 指令（directives）

支持 Cobra-like directives（例如 no-file-comp、keep-order、文件扩展名过滤、目录过滤等）。

常用辅助方法：

- `markFlagFilename("--config", {"yaml", "yml"})`
- `markFlagDirname("--out-dir")`
- `markPersistentFlagFilename("--config", {"yaml", "yml"})`
- `markPersistentFlagDirname("--out-dir")`

这些方法会输出供 bash/zsh/fish/powershell 补全脚本消费的 directive 元数据。
