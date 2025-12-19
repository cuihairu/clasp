# TODO (Cobra-Like Feature Checklist)

Goal: implement a C++17 CLI framework that mirrors Go cobra's behavior and ergonomics.

## P0 Core (Make It Usable)

- [x] Command tree
- [x] `Use/Short/Long` metadata
- [x] Subcommand registration + lookup
- [x] `Run/RunE` (return exit code / error)
- [x] Args validation helpers (NoArgs / ExactArgs / MinimumNArgs / MaximumNArgs / RangeArgs)
- [x] Basic flag parsing
- [x] `--long` / `-s`
- [x] `--k=v`
- [x] `--` end-of-flags
- [x] bool/string/int/float values + defaults
- [x] Help & usage
- [x] `--help/-h`
- [x] `help [cmd]` subcommand
- [x] Usage template: Usage / Commands / Flags
- [x] Version
- [x] `--version` and/or `version` subcommand

## P1 Usability (Closer To Cobra)

- [x] Hooks
- [x] `PreRun/PreRunE/PostRun/PostRunE`
- [x] `PersistentPreRun/...` (inherit)
- [x] Persistent flags (inherit from parents)
- [x] Local vs persistent flag display
- [x] Required/hidden/deprecated
- [x] Required flags
- [x] Hidden flags/commands
- [x] Deprecated messages
- [ ] Error/usage behavior
- [x] `SilenceUsage`
- [x] `SilenceErrors`
- [x] Suggestions
- [x] Unknown command suggestions (closest match)
- [x] Flag suggestions (closest match)
- [ ] Flag parsing details (optional)
- [x] Short flag grouping (e.g. `-abc`)
- [x] Bool negation (optional, e.g. `--no-foo`)

## P2 Optional Ecosystem

- [ ] Shell completion
- [ ] bash/zsh/fish/powershell completion generation
- [ ] Docs generation
- [ ] markdown/manpage generation
- [ ] Config integration (viper-like, optional)
- [ ] env/config file binding + precedence merge
