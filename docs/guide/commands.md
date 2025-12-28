# Commands（命令树）

## 子命令与别名

- `addCommand(Command)`：挂子命令
- `aliases({...}) / addAlias()`：命令别名

## Hooks（生命周期）

Clasp 提供 Cobra-like hooks：

- `preRun/preRunE/postRun/postRunE`
- `persistentPreRun/persistentPreRunE/persistentPostRun/persistentPostRunE`（向下继承）

## Args 校验

通过 `cmd.args(validator)`：

- `NoArgs()` / `ExactArgs(n)` / `MinimumNArgs(n)` / `MaximumNArgs(n)` / `RangeArgs(min,max)`

## 执行模式

- CLI 入口：`run(argc, argv)`
- 程序内执行：`setArgs(vector<string>)` + `execute()`（适合测试/嵌入式场景）
