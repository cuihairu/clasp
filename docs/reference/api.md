# API Quick Reference

## Command

- Construction: `Command(name, short, long?)`
- Subcommands: `addCommand(cmd)`
- Flags: `withFlag/withPersistentFlag`
- pflag semantics: `markFlagNoOptDefaultValue()`, `shortFlagGrouping()`, `boolNegation()`
- Hooks: `preRun/preRunE/postRun/postRunE` + persistent variants
- Args: `args(validator)`
- Execution: `run(argc, argv)` / `setArgs(vec)` + `execute()`
- Output control: `silenceUsage/silenceErrors`, `setOut/setErr`
- help/usage/version: `setHelpTemplate/setUsageTemplate/setVersionTemplate`, `setHelpFunc/setUsageFunc`
- Completion: `enableCompletion(...)`, `validArgs/validArgsFunction/registerFlagCompletion`

## Parser

- Scalars: `getFlag<T>("--flag", default)`
- Occurrences: `occurrences("--flag")`
- pflag helpers: `getStringSlice/getIntSlice/...`, `getStringToInt/...`

## Value

- `type/string/set` + `bindFlagValue/withValueFlag`
