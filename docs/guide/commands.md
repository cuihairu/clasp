# Commands (Command Tree)

## Subcommands and Aliases

- `addCommand(Command)`: Add subcommands
- `aliases({...}) / addAlias()`: Command aliases

## Hooks (Lifecycle)

Clasp provides Cobra-like hooks:

- `preRun/preRunE/postRun/postRunE`
- `persistentPreRun/persistentPreRunE/persistentPostRun/persistentPostRunE` (inherited by child commands)

## Args Validation

Use `cmd.args(validator)`:

- `NoArgs()` / `ExactArgs(n)` / `MinimumNArgs(n)` / `MaximumNArgs(n)` / `RangeArgs(min,max)`

## Execution Modes

- CLI entry: `run(argc, argv)`
- In-process execution: `setArgs(vector<string>)` + `execute()` (suitable for testing/embedded scenarios)
