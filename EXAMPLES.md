# Examples

All examples are built as executables and exercised via CTest from `CMakeLists.txt`.

## Build & Run

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Example Index

- `examples/basic_example.cpp`: Basic command + flags + alias + version/help.
- `examples/hooks_example.cpp`: Hook ordering (`PersistentPreRun`/`PreRun`/`PostRun`).
- `examples/required_example.cpp`: Required flags.
- `examples/flag_groups_example.cpp`: Mutually-exclusive / one-required / required-together groups.
- `examples/traverse_example.cpp`: `TraverseChildren` (child flags before subcommands).
- `examples/no_suggest_example.cpp`: Disable suggestions.
- `examples/docs_example.cpp`: Markdown + manpage generation.
- `examples/completion_example.cpp`: Shell completion generation (bash/zsh/fish/powershell).
- `examples/dynamic_completion_example.cpp`: Dynamic completions (valid args + flag value completion).
- `examples/directive_example.cpp`: `__complete` directives (special `:N` line).
- `examples/file_completion_example.cpp`: File/dir completion directives (`markFlagFilename` / `markFlagDirname`).
- `examples/keep_order_example.cpp`: Completion `KeepOrder` directive (disables shell sorting).
- `examples/completion_naming_example.cpp`: Custom completion command names / disable defaults.
- `examples/parser_knobs_example.cpp`: Parser knobs (unknown flags, short grouping, bool negation).
- `examples/normalize_example.cpp`: Flag key normalization (underscore ↔ dash).
- `examples/noopt_example.cpp`: `NoOptDefVal` (non-bool flag optional value).
- `examples/repeat_example.cpp`: Repeated flags + CSV-style split helper.
- `examples/map_example.cpp`: Map-style helper (`a=1,b=2`) built on repeated/split values.
- `examples/types_example.cpp`: Extra flag types (`int64/uint64/double/duration`).
- `examples/external_typed_example.cpp`: Env-bound typed flags validated strictly (env value parse errors fail early).
- `examples/bytes_example.cpp`: Bytes flag (`withPersistentBytesFlag`, values like `1KB`, `1.5MiB`).
- `examples/pflag_types_example.cpp`: pflag-like getters (`getStringSlice/getStringArray/getStringToString`).
- `examples/custom_value_example.cpp`: Custom flag Value interface (`Value::set/string/type`).
- `examples/silence_errors_example.cpp`: `SilenceErrors` suppresses unknown-command output.
- `examples/version_template_example.cpp`: Custom version template (`setVersionTemplate`).
- `examples/disable_flags_in_use_line_example.cpp`: Hide `[flags]` in the usage line (`disableFlagsInUseLine`).
- `examples/flag_error_func_example.cpp`: Customize flag parse errors (`setFlagErrorFunc`).
- `examples/help_command_rename_example.cpp`: Rename the built-in help command (`enableHelp`).
- `examples/help_command_disabled_example.cpp`: Disable the built-in help command (`disableHelpCommand`).
- `examples/count_example.cpp`: Count flags (`-vvv` style) via `withCountFlag` + `Parser::getCount`.
- `examples/context_example.cpp`: Inherited command context (`setContext` / `runWithContext`).
- `examples/config_example.cpp`: `.env`-style config + env binding precedence merge.
- `examples/config_json_example.cpp` + `examples/config.json`: JSON config (nested flatten).
- `examples/config_toml_example.cpp` + `examples/config.toml`: TOML config (basic tables + flatten).
- `examples/config_yaml_example.cpp` + `examples/config.yaml`: YAML config (basic nested mappings + flatten).
- `examples/config_list_example.cpp` + `examples/config_list.*`: Multi-value config arrays/sequences mapped to repeated flags.
- `examples/sorting_example.cpp` / `examples/no_sorting_example.cpp`: Command/flag sorting toggles.
- `examples/execute_example.cpp`: Programmatic execution (`setArgs` + `execute()`).
