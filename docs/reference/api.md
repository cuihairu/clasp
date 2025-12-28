# API 速查

## Command

- 构造：`Command(name, short, long?)`
- 子命令：`addCommand(cmd)`
- flags：`withFlag/withPersistentFlag`
- hooks：`preRun/preRunE/postRun/postRunE` + persistent variants
- args：`args(validator)`
- 执行：`run(argc, argv)` / `setArgs(vec)` + `execute()`
- 输出控制：`silenceUsage/silenceErrors`、`setOut/setErr`
- help/usage/version：`setHelpTemplate/setUsageTemplate/setVersionTemplate`、`setHelpFunc/setUsageFunc`
- completion：`enableCompletion(...)`、`validArgs/validArgsFunction/registerFlagCompletion`

## Parser

- 标量：`getFlag<T>("--flag", default)`
- occurrences：`occurrences("--flag")`
- pflag helpers：`getStringSlice/getIntSlice/...`、`getStringToInt/...`

## Value

- `type/string/set` + `bindFlagValue/withValueFlag`
