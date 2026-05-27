# Examples

所有 examples 都在仓库根目录的 `examples/`，并通过 CTest 执行（`CMakeLists.txt`）。

## 推荐路径

- 从 `basic_example` 开始：命令树、flags、help/version。
- 需要对齐 parsing 细节：看 `parser_knobs_example` / `repeat_example` / `noopt_example`。
- 需要 completion：看 `completion_example` / `dynamic_completion_example` / `directive_example`。
- 需要 env/config 合并：看 `config_*` / `external_typed_example`。

## 示例索引

> 下面的文件名均位于 `examples/`。

- `basic_example.cpp`: Basic command + flags + alias + version/help.
- `args_example.cpp`: Args validators (`NoArgs/ExactArgs/RangeArgs`) + `SilenceUsage` / `SilenceErrors`.
- `hooks_example.cpp`: Hook ordering (`PersistentPreRun`/`PreRun`/`PostRun`).
- `required_example.cpp`: Required flags.
- `flag_groups_example.cpp`: Mutually-exclusive / one-required / required-together groups.
- `traverse_example.cpp`: `TraverseChildren`.
- `no_suggest_example.cpp`: Disable suggestions.
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
- `custom_value_example.cpp`: Custom flag `Value` interface.
- `parser_external_example.cpp`: Parser external values (multi-source merge + count flags).
- `silence_errors_example.cpp`: `SilenceErrors` suppresses unknown-command output.
- `version_template_example.cpp`: Custom version template (`setVersionTemplate`).
- `disable_flags_in_use_line_example.cpp`: Hide `[flags]` in usage line.
- `flag_error_func_example.cpp`: Customize flag parse errors (`setFlagErrorFunc`).
- `help_command_rename_example.cpp`: Rename built-in help command.
- `help_command_disabled_example.cpp`: Disable built-in help command.
- `count_example.cpp`: Count flags (`-vvv`) via `withCountFlag` + `Parser::getCount`.
- `context_example.cpp`: Inherited command context.
- `config_example.cpp`: `.env`-style config + env binding precedence merge.
- `config_json_example.cpp`: JSON config (nested flatten).
- `config_toml_example.cpp`: TOML config.
- `config_yaml_example.cpp`: YAML config.
- `config_list_example.cpp`: Config arrays/sequences mapped to repeated flags.
- `sorting_example.cpp` / `no_sorting_example.cpp`: Sorting toggles.
- `execute_example.cpp`: Programmatic execution.
- `hook_error_example.cpp`: Error-returning hooks and `actionE`.
- `api_coverage_example.cpp`: Builder-style API surface coverage.
