# API 速查

## Command

- 构造：`Command(name, short, long?)`
- 子命令：`addCommand(cmd)`
- flags：`withFlag/withPersistentFlag`
- pflag 语义：`markFlagNoOptDefaultValue()/markPersistentFlagNoOptDefaultValue()`、`shortFlagGrouping()`、`boolNegation()`
- 补全辅助：`markFlagFilename()/markPersistentFlagFilename()`、`markFlagDirname()/markPersistentFlagDirname()`
- hooks：`preRun/preRunE/postRun/postRunE` + persistent variants
- args：`args(validator)`
- 执行：`run(argc, argv)` / `setArgs(vec)` + `execute()`
- 输出控制：`silenceUsage/silenceErrors`、`setOut/setErr`
- help/usage/version：`setHelpTemplate/setUsageTemplate/setVersionTemplate`、`setHelpFunc/setUsageFunc`
- completion：`enableCompletion(...)`、`validArgs/validArgsFunction/registerFlagCompletion`
- 外部源：`bindEnv()`、`configFile()`、`configFileFlag()`

## Parser

- 标量：`getFlag<T>("--flag", default)`
- occurrences：`occurrences("--flag")`
- pflag helpers：`getStringSlice/getIntSlice/...`、`getStringToInt/...`

## Value

- `type/string/set` + `bindFlagValue/withValueFlag`
