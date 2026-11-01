# Examples

All examples are in `examples/` at the repository root and executed via CTest (`CMakeLists.txt`).

## Recommended Path

- Start with `basic_example`: command tree, flags, help/version.
- For parsing details: see `parser_knobs_example` / `repeat_example` / `noopt_example`.
- For completion: see `completion_example` / `dynamic_completion_example` / `directive_example`.
- For env/config merging: see `config_*` / `external_typed_example`.

## Example Index

> Filenames below are located in `examples/`.

- `basic_example.cpp`: Basic command + flags + alias + version/help.
- `args_example.cpp`: Args validators (`NoArgs/ExactArgs/RangeArgs`) + `SilenceUsage` / `SilenceErrors`.
- `hooks_example.cpp`: Hook ordering (`PersistentPreRun`/`PreRun`/`PostRun`).
- `required_example.cpp`: Required flags.
- `deprecated_example.cpp`: Deprecated command/flag warnings (`deprecated()`).
- `flag_groups_example.cpp`: Mutually-exclusive / one-required / required-together groups.
- `group_example.cpp`: Command groups (`addGroup` / `groupId`; unknown group treated as ungrouped).
- `traverse_example.cpp`: `TraverseChildren`.
- `traverse_completion_example.cpp`: `TraverseChildren` + completion (subcommand token not consumed as a flag value).
- `no_suggest_example.cpp`: Disable suggestions.
- `utils_suggest_example.cpp`: `utils::suggest` prefix/distance suggestions.
- `docs_example.cpp`: Markdown + manpage generation.
- `completion_example.cpp`: Completion script generation (bash/zsh/fish/powershell).
- `dynamic_completion_example.cpp`: Dynamic completions (valid args + flag value completion).
- `directive_example.cpp`: `__complete` directives (`:N` line).
- `file_completion_example.cpp`: File/dir completion directives (`markFlagFilename` / `markFlagDirname`).
- `keep_order_example.cpp`: Completion `KeepOrder` directive (disable shell sorting).
- `completion_naming_example.cpp`: Custom completion command names / disable defaults.
- `parser_knobs_example.cpp`: Parser knobs (unknown flags, short grouping, bool negation).
- `normalize_example.cpp`: Flag key normalization (underscore ↔ dash).
- `noopt_example.cpp`: `NoOptDefVal` (non-bool flag optional value).
- `repeat_example.cpp`: Repeated flags + split helper.
- `map_example.cpp`: Map-style helper (`a=1,b=2`) built on repeated/split values.
- `float_flag_example.cpp`: Float flag parsing (valid + invalid).
- `types_example.cpp`: Extra flag types (`int64/uint64/double/duration`).
- `net_example.cpp`: IP + CIDR flag types (`withIPFlag` / `withCIDRFlag`).
- `net_extra_example.cpp`: IPNet + IPMask flag types (`withIPNetFlag` / `withIPMaskFlag`).
- `url_example.cpp`: URL flag type (`withURLFlag`).
- `external_typed_example.cpp`: Env-bound typed flags validated strictly.
- `bytes_example.cpp`: Bytes flag (`withPersistentBytesFlag`).
- `pflag_types_example.cpp`: pflag-like getters.
- `pflag_types2_example.cpp`: more pflag types — narrow widths (`withInt8Flag`/`withUint16Flag`/`withUintFlag`...), `withIPSliceFlag`, `withBytesBase64Flag`, `getStringToFloat`.
- `cobra_strings_example.cpp`: canonical Cobra-like error/usage strings locked byte-for-byte (Must Support contract).
- `pflag_edge_example.cpp`: pflag edge interactions (repeated bools, empty `=` values, negative numbers, intermixed positionals, `NoOptDefVal` forms, `--`).
- `custom_value_example.cpp`: Custom flag `Value` interface.
- `parser_external_example.cpp`: Parser external values (multi-source merge + count flags).
- `silence_errors_example.cpp`: `SilenceErrors` suppresses unknown-command output.
- `version_template_example.cpp`: Custom version template (`setVersionTemplate`).
- `custom_template_example.cpp`: Custom help/usage templates (`setHelpTemplate` / `setUsageTemplate`).
- `disable_flags_in_use_line_example.cpp`: Hide `[flags]` in usage line.
- `flag_error_func_example.cpp`: Customize flag parse errors (`setFlagErrorFunc`).
- `flag_convert_example.cpp`: `Flag::convertToFlagValue` type conversion (bool/int/int64/uint32/uint64/float/double/duration/string).
- `help_command_rename_example.cpp`: Rename built-in help command.
- `help_command_disabled_example.cpp`: Disable built-in help command.
- `count_example.cpp`: Count flags (`-vvv`) via `withCountFlag` + `Parser::getCount`.
- `context_example.cpp`: Inherited command context.
- `config_example.cpp`: `.env`-style config + env binding precedence merge.
- `config_ini_example.cpp`: `.ini` / `.cfg` config (section keys flattened).
- `config_json_example.cpp`: JSON config (nested flatten).
- `config_toml_example.cpp`: TOML config.
- `config_yaml_example.cpp`: YAML config.
- `config_list_example.cpp`: Config arrays/sequences mapped to repeated flags.
- `sorting_example.cpp` / `no_sorting_example.cpp`: Sorting toggles.
- `execute_example.cpp`: Programmatic execution.
- `hook_error_example.cpp`: Error-returning hooks and `actionE`.

Note: builder-style API surface coverage lives in `tests/api.cpp` (built as `api_test`, CTest `api_coverage`), not in `examples/`.
