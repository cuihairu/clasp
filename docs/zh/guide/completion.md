# Completion（补全）

## 生成脚本

Clasp 支持 bash/zsh/fish/powershell completion 脚本生成（对应 `examples/completion_example.cpp`）。

## 动态补全（__complete）

- positional：`validArgs()` / `validArgsFunction()`
- flag value：`registerFlagCompletion("--flag", func)`

## 指令（directives）

支持 Cobra-like directives（例如 no-file-comp、keep-order、文件扩展名过滤、目录过滤等）。
