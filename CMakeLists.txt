cmake_minimum_required(VERSION 3.14)
project(clasp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
enable_testing()

add_library(clasp
        src/clasp.cpp
        src/command.cpp
        src/flag.cpp
)

target_include_directories(clasp PUBLIC include)

add_executable(basic_example examples/basic_example.cpp)
target_link_libraries(basic_example PRIVATE clasp)

add_test(NAME basic_help COMMAND basic_example help)
set_tests_properties(basic_help PROPERTIES PASS_REGULAR_EXPRESSION "Usage: app")

add_test(NAME basic_help_sub COMMAND basic_example help print)
set_tests_properties(basic_help_sub PROPERTIES PASS_REGULAR_EXPRESSION "Usage: app print")

add_test(NAME basic_help_examples COMMAND basic_example --help)
set_tests_properties(basic_help_examples PROPERTIES PASS_REGULAR_EXPRESSION "Examples:")

add_test(NAME basic_help_shows_defaults COMMAND basic_example help print)
set_tests_properties(basic_help_shows_defaults PROPERTIES PASS_REGULAR_EXPRESSION "\\(default: \\\"Hello, World!\\\"\\)")

add_test(NAME basic_help_shows_types COMMAND basic_example help print)
set_tests_properties(basic_help_shows_types PROPERTIES PASS_REGULAR_EXPRESSION "--message string")

add_test(NAME basic_unknown COMMAND basic_example nope)
set_tests_properties(basic_unknown PROPERTIES PASS_REGULAR_EXPRESSION "unknown command")

add_test(NAME basic_missing_flag_value COMMAND basic_example print --message)
set_tests_properties(basic_missing_flag_value PROPERTIES PASS_REGULAR_EXPRESSION "flag needs an argument.*Usage: app print")

add_test(NAME basic_unknown_cmd_suggest COMMAND basic_example prnit)
set_tests_properties(basic_unknown_cmd_suggest PROPERTIES PASS_REGULAR_EXPRESSION [==[Did you mean this\?
  print]==])

add_test(NAME basic_unknown_flag_suggest COMMAND basic_example print --mesage hi)
set_tests_properties(basic_unknown_flag_suggest PROPERTIES PASS_REGULAR_EXPRESSION [==[Did you mean this\?
  --message]==])

add_test(NAME basic_persistent_flag_before_sub COMMAND basic_example --verbose print --message hi)
set_tests_properties(basic_persistent_flag_before_sub PROPERTIES PASS_REGULAR_EXPRESSION "hi")

add_test(NAME basic_short_group COMMAND basic_example -vq print --message hi)
set_tests_properties(basic_short_group PROPERTIES PASS_REGULAR_EXPRESSION "hi")

add_test(NAME basic_bool_negation COMMAND basic_example --no-verbose print --message hi)
set_tests_properties(basic_bool_negation PROPERTIES PASS_REGULAR_EXPRESSION "hi")

add_test(NAME basic_short_value_inline COMMAND basic_example print -mhi)
set_tests_properties(basic_short_value_inline PROPERTIES PASS_REGULAR_EXPRESSION "hi")

add_test(NAME basic_short_value_eq COMMAND basic_example print -m=hi)
set_tests_properties(basic_short_value_eq PROPERTIES PASS_REGULAR_EXPRESSION "hi")

add_test(NAME basic_end_of_flags COMMAND basic_example print -- --message hi)
set_tests_properties(basic_end_of_flags PROPERTIES PASS_REGULAR_EXPRESSION "Hello, World!")

add_test(NAME basic_end_of_flags_preserves_before COMMAND basic_example print --message hi -- --message bye)
set_tests_properties(basic_end_of_flags_preserves_before PROPERTIES PASS_REGULAR_EXPRESSION "hi")

add_test(NAME basic_alias COMMAND basic_example p --message hi)
set_tests_properties(basic_alias PROPERTIES PASS_REGULAR_EXPRESSION "hi")

add_executable(args_example examples/args_example.cpp)
target_link_libraries(args_example PRIVATE clasp)

add_test(NAME args_noargs_ok COMMAND args_example noargs)
set_tests_properties(args_noargs_ok PROPERTIES PASS_REGULAR_EXPRESSION "ok")

add_test(NAME args_noargs_fail_shows_usage COMMAND args_example noargs x)
set_tests_properties(args_noargs_fail_shows_usage PROPERTIES PASS_REGULAR_EXPRESSION "Error: accepts no arguments.*Usage: app noargs")

add_test(NAME args_exact_fail_shows_usage COMMAND args_example exact one)
set_tests_properties(args_exact_fail_shows_usage PROPERTIES PASS_REGULAR_EXPRESSION "Error: accepts 2 arg\\(s\\).*Usage: app exact")

add_test(NAME args_range_fail_shows_usage COMMAND args_example range a b c)
set_tests_properties(args_range_fail_shows_usage PROPERTIES PASS_REGULAR_EXPRESSION "Error: accepts between 1 and 2 arg\\(s\\).*Usage: app range")

add_test(NAME args_silence_usage_hides_usage COMMAND args_example silence_usage)
set_tests_properties(args_silence_usage_hides_usage PROPERTIES PASS_REGULAR_EXPRESSION "Error: accepts 1 arg\\(s\\)" FAIL_REGULAR_EXPRESSION "Usage:")

add_test(NAME args_silence_errors_hides_error COMMAND args_example silence_errors)
set_tests_properties(args_silence_errors_hides_error PROPERTIES PASS_REGULAR_EXPRESSION "Usage: app silence_errors" FAIL_REGULAR_EXPRESSION "Error:")

add_test(NAME basic_invalid_bool_eq COMMAND basic_example --verbose=maybe print --message hi)
set_tests_properties(basic_invalid_bool_eq PROPERTIES PASS_REGULAR_EXPRESSION [==[invalid argument "maybe" for "--verbose".*Usage: app print]==])

add_test(NAME basic_disable_flag_parsing COMMAND basic_example raw --help --x -y)
set_tests_properties(basic_disable_flag_parsing PROPERTIES PASS_REGULAR_EXPRESSION "--help --x -y")

add_test(NAME basic_version_flag COMMAND basic_example --version)
set_tests_properties(basic_version_flag PROPERTIES PASS_REGULAR_EXPRESSION "0.1.0")

add_test(NAME basic_version_cmd COMMAND basic_example version)
set_tests_properties(basic_version_cmd PROPERTIES PASS_REGULAR_EXPRESSION "0.1.0")

add_executable(hooks_example examples/hooks_example.cpp)
target_link_libraries(hooks_example PRIVATE clasp)

add_test(NAME hooks_order COMMAND hooks_example parent child)
set_tests_properties(hooks_order PROPERTIES PASS_REGULAR_EXPRESSION "root-pre.*parent-pre.*child-pre.*run.*child-post.*parent-post.*root-post")

add_executable(required_example examples/required_example.cpp)
target_link_libraries(required_example PRIVATE clasp)

add_test(NAME required_missing COMMAND required_example do)
set_tests_properties(required_missing PROPERTIES PASS_REGULAR_EXPRESSION "required flag not set.*Usage: app do")

add_test(NAME required_ok COMMAND required_example do --name bob)
set_tests_properties(required_ok PROPERTIES PASS_REGULAR_EXPRESSION "bob")

add_test(NAME required_empty_value_ok COMMAND required_example do --name=)

add_executable(docs_example examples/docs_example.cpp)
target_link_libraries(docs_example PRIVATE clasp)

add_test(NAME docs_markdown COMMAND docs_example)
set_tests_properties(docs_markdown PROPERTIES PASS_REGULAR_EXPRESSION [==[# app]==])

add_test(NAME docs_manpage COMMAND docs_example)
set_tests_properties(docs_manpage PROPERTIES PASS_REGULAR_EXPRESSION [==[MANPAGE
.TH "app" "1"]==])

add_executable(completion_example examples/completion_example.cpp)
target_link_libraries(completion_example PRIVATE clasp)

add_test(NAME completion_bash COMMAND completion_example completion bash)
set_tests_properties(completion_bash PROPERTIES PASS_REGULAR_EXPRESSION "complete -F")

add_test(NAME completion_bash_dynamic COMMAND completion_example completion bash)
set_tests_properties(completion_bash_dynamic PROPERTIES PASS_REGULAR_EXPRESSION "__completeNoDesc")

add_test(NAME completion_bash_directive_filter_dirs COMMAND completion_example completion bash)
set_tests_properties(completion_bash_directive_filter_dirs PROPERTIES PASS_REGULAR_EXPRESSION "compgen -d")

add_test(NAME completion_bash_directive_filter_ext COMMAND completion_example completion bash)
set_tests_properties(completion_bash_directive_filter_ext PROPERTIES PASS_REGULAR_EXPRESSION "compgen -f -X")

add_test(NAME completion_bash_directive_keep_order COMMAND completion_example completion bash)
set_tests_properties(completion_bash_directive_keep_order PROPERTIES PASS_REGULAR_EXPRESSION "compopt -o nosort")

add_test(NAME completion_bash_eq_split COMMAND completion_example completion bash)
set_tests_properties(completion_bash_eq_split PROPERTIES PASS_REGULAR_EXPRESSION "cur%%=\\*")

add_test(NAME completion_zsh COMMAND completion_example completion zsh)
set_tests_properties(completion_zsh PROPERTIES PASS_REGULAR_EXPRESSION "bashcompinit")

add_test(NAME completion_zsh_dynamic COMMAND completion_example completion zsh)
set_tests_properties(completion_zsh_dynamic PROPERTIES PASS_REGULAR_EXPRESSION "__completeNoDesc")

add_test(NAME completion_fish COMMAND completion_example completion fish)
set_tests_properties(completion_fish PROPERTIES PASS_REGULAR_EXPRESSION "complete -c app")

add_test(NAME completion_fish_dynamic COMMAND completion_example completion fish)
set_tests_properties(completion_fish_dynamic PROPERTIES PASS_REGULAR_EXPRESSION "__complete")

add_test(NAME completion_fish_dynamic_directives COMMAND completion_example completion fish)
set_tests_properties(completion_fish_dynamic_directives PROPERTIES PASS_REGULAR_EXPRESSION "__fish_complete_directories.*command ls -1")

add_test(NAME completion_powershell COMMAND completion_example completion powershell)
set_tests_properties(completion_powershell PROPERTIES PASS_REGULAR_EXPRESSION "Register-ArgumentCompleter")

add_test(NAME completion_powershell_dynamic COMMAND completion_example completion powershell)
set_tests_properties(completion_powershell_dynamic PROPERTIES PASS_REGULAR_EXPRESSION "__completeNoDesc")

add_test(NAME completion_powershell_eq_split COMMAND completion_example completion powershell)
set_tests_properties(completion_powershell_eq_split PROPERTIES PASS_REGULAR_EXPRESSION "\\$origWordToComplete")

add_executable(file_completion_example examples/file_completion_example.cpp)
target_link_libraries(file_completion_example PRIVATE clasp)

add_test(NAME dynamic_complete_flag_filename_candidates COMMAND file_completion_example __completeNoDesc open --config y)
set_tests_properties(dynamic_complete_flag_filename_candidates PROPERTIES PASS_REGULAR_EXPRESSION "yaml")

add_test(NAME dynamic_complete_flag_filename_candidates_path_prefix COMMAND file_completion_example __completeNoDesc open --config /)
set_tests_properties(dynamic_complete_flag_filename_candidates_path_prefix PROPERTIES PASS_REGULAR_EXPRESSION "yaml")

add_test(NAME dynamic_complete_flag_filename_directive COMMAND file_completion_example __completeNoDesc open --config y)
set_tests_properties(dynamic_complete_flag_filename_directive PROPERTIES PASS_REGULAR_EXPRESSION ":8")

add_test(NAME dynamic_complete_flag_dirname_directive COMMAND file_completion_example __completeNoDesc open --out-dir a)
set_tests_properties(dynamic_complete_flag_dirname_directive PROPERTIES PASS_REGULAR_EXPRESSION ":16")

add_test(NAME completion_fish_dynamic_filter_dirs COMMAND file_completion_example completion fish)
set_tests_properties(completion_fish_dynamic_filter_dirs PROPERTIES PASS_REGULAR_EXPRESSION "__fish_complete_directories")

add_test(NAME completion_fish_dynamic_filter_ext COMMAND file_completion_example completion fish)
set_tests_properties(completion_fish_dynamic_filter_ext PROPERTIES PASS_REGULAR_EXPRESSION "command ls -1")

add_test(NAME completion_fish_eq_split COMMAND file_completion_example completion fish)
set_tests_properties(completion_fish_eq_split PROPERTIES PASS_REGULAR_EXPRESSION "string split -m1 '='.*set prefix.*echo \\$prefix")

add_executable(keep_order_example examples/keep_order_example.cpp)
target_link_libraries(keep_order_example PRIVATE clasp)

add_test(NAME keep_order_directive_bitmask COMMAND keep_order_example __completeNoDesc fruit a)
set_tests_properties(keep_order_directive_bitmask PROPERTIES PASS_REGULAR_EXPRESSION ":36")

add_test(NAME keep_order_fish_script_uses_keep_order COMMAND keep_order_example completion fish)
set_tests_properties(keep_order_fish_script_uses_keep_order PROPERTIES PASS_REGULAR_EXPRESSION "complete -c app -f -k")

add_executable(config_example examples/config_example.cpp)
target_link_libraries(config_example PRIVATE clasp)

add_test(NAME config_from_file COMMAND config_example do --config ${CMAKE_CURRENT_SOURCE_DIR}/examples/config.env)
set_tests_properties(config_from_file PROPERTIES PASS_REGULAR_EXPRESSION "fromfile")

add_test(NAME config_env_overrides COMMAND config_example do --config ${CMAKE_CURRENT_SOURCE_DIR}/examples/config.env)
set_tests_properties(config_env_overrides PROPERTIES ENVIRONMENT "APP_MESSAGE=fromenv" PASS_REGULAR_EXPRESSION "fromenv")

add_test(NAME config_flag_overrides_env COMMAND config_example do --config ${CMAKE_CURRENT_SOURCE_DIR}/examples/config.env --message fromcli)
set_tests_properties(config_flag_overrides_env PROPERTIES ENVIRONMENT "APP_MESSAGE=fromenv" PASS_REGULAR_EXPRESSION "fromcli")

add_executable(config_json_example examples/config_json_example.cpp)
target_link_libraries(config_json_example PRIVATE clasp)

add_test(NAME config_from_json COMMAND config_json_example do --config ${CMAKE_CURRENT_SOURCE_DIR}/examples/config.json)
set_tests_properties(config_from_json PROPERTIES PASS_REGULAR_EXPRESSION "message=fromjson.*nested_mode=deep")

add_executable(config_toml_example examples/config_toml_example.cpp)
target_link_libraries(config_toml_example PRIVATE clasp)

add_test(NAME config_from_toml COMMAND config_toml_example do --config ${CMAKE_CURRENT_SOURCE_DIR}/examples/config.toml)
set_tests_properties(config_from_toml PROPERTIES PASS_REGULAR_EXPRESSION "message=fromtoml.*nested_mode=deep")

add_executable(config_yaml_example examples/config_yaml_example.cpp)
target_link_libraries(config_yaml_example PRIVATE clasp)

add_test(NAME config_from_yaml COMMAND config_yaml_example do --config ${CMAKE_CURRENT_SOURCE_DIR}/examples/config.yaml)
set_tests_properties(config_from_yaml PROPERTIES PASS_REGULAR_EXPRESSION "message=fromyaml.*nested_mode=deep")

add_executable(config_list_example examples/config_list_example.cpp)
target_link_libraries(config_list_example PRIVATE clasp)

add_test(NAME config_list_from_json COMMAND config_list_example do --config ${CMAKE_CURRENT_SOURCE_DIR}/examples/config_list.json)
set_tests_properties(config_list_from_json PROPERTIES PASS_REGULAR_EXPRESSION "tags=a,b,c")

add_test(NAME config_list_from_yaml COMMAND config_list_example do --config ${CMAKE_CURRENT_SOURCE_DIR}/examples/config_list.yaml)
set_tests_properties(config_list_from_yaml PROPERTIES PASS_REGULAR_EXPRESSION "tags=a,b,c")

add_test(NAME config_list_from_toml COMMAND config_list_example do --config ${CMAKE_CURRENT_SOURCE_DIR}/examples/config_list.toml)
set_tests_properties(config_list_from_toml PROPERTIES PASS_REGULAR_EXPRESSION "tags=a,b,c")

add_test(NAME config_list_cli_overrides COMMAND config_list_example do --config ${CMAKE_CURRENT_SOURCE_DIR}/examples/config_list.json --tag x)
set_tests_properties(config_list_cli_overrides PROPERTIES PASS_REGULAR_EXPRESSION "tags=x")

add_executable(traverse_example examples/traverse_example.cpp)
target_link_libraries(traverse_example PRIVATE clasp)

add_test(NAME traverse_local_flag_before_sub COMMAND traverse_example --message hi print)
set_tests_properties(traverse_local_flag_before_sub PROPERTIES PASS_REGULAR_EXPRESSION "hi")

add_executable(flag_groups_example examples/flag_groups_example.cpp)
target_link_libraries(flag_groups_example PRIVATE clasp)

add_test(NAME flag_groups_mutually_exclusive COMMAND flag_groups_example do --a --b --name bob --user u --pass p)
set_tests_properties(flag_groups_mutually_exclusive PROPERTIES PASS_REGULAR_EXPRESSION "mutually exclusive")

add_test(NAME flag_groups_one_required COMMAND flag_groups_example do --user u --pass p)
set_tests_properties(flag_groups_one_required PROPERTIES PASS_REGULAR_EXPRESSION "at least one of the flags in the group is required")

add_test(NAME flag_groups_required_together COMMAND flag_groups_example do --name bob --user u)
set_tests_properties(flag_groups_required_together PROPERTIES PASS_REGULAR_EXPRESSION "flags must be set together")

add_test(NAME flag_groups_ok COMMAND flag_groups_example do --a --name bob --user u --pass p)
set_tests_properties(flag_groups_ok PROPERTIES PASS_REGULAR_EXPRESSION "ok a=true")

add_executable(no_suggest_example examples/no_suggest_example.cpp)
target_link_libraries(no_suggest_example PRIVATE clasp)

add_test(NAME no_suggest_unknown_flag COMMAND no_suggest_example print --mesage hi)
set_tests_properties(no_suggest_unknown_flag PROPERTIES PASS_REGULAR_EXPRESSION "unknown flag" FAIL_REGULAR_EXPRESSION "Did you mean this\\?")

add_executable(dynamic_completion_example examples/dynamic_completion_example.cpp)
target_link_libraries(dynamic_completion_example PRIVATE clasp)

add_test(NAME dynamic_complete_subcommands COMMAND dynamic_completion_example __completeNoDesc p)
set_tests_properties(dynamic_complete_subcommands PROPERTIES PASS_REGULAR_EXPRESSION "paint")

add_test(NAME dynamic_complete_valid_args COMMAND dynamic_completion_example __completeNoDesc fruit a)
set_tests_properties(dynamic_complete_valid_args PROPERTIES PASS_REGULAR_EXPRESSION "apple")

add_test(NAME dynamic_complete_flag_value COMMAND dynamic_completion_example __completeNoDesc paint --color r)
set_tests_properties(dynamic_complete_flag_value PROPERTIES PASS_REGULAR_EXPRESSION "red")

add_executable(parser_knobs_example examples/parser_knobs_example.cpp)
target_link_libraries(parser_knobs_example PRIVATE clasp)

add_test(NAME parser_knobs_allow_unknown_consumes_value COMMAND parser_knobs_example unknown_ok --mystery value)
set_tests_properties(parser_knobs_allow_unknown_consumes_value PROPERTIES PASS_REGULAR_EXPRESSION "verbose=false args=\\[\\]")

add_test(NAME parser_knobs_allow_unknown_does_not_consume_other_flags COMMAND parser_knobs_example unknown_ok --mystery -v)
set_tests_properties(parser_knobs_allow_unknown_does_not_consume_other_flags PROPERTIES PASS_REGULAR_EXPRESSION "verbose=true args=\\[\\]")

add_test(NAME parser_knobs_allow_unknown_eq_consumes_token_only COMMAND parser_knobs_example unknown_ok --mystery=val leftover)
set_tests_properties(parser_knobs_allow_unknown_eq_consumes_token_only PROPERTIES PASS_REGULAR_EXPRESSION "verbose=false args=\\[leftover\\]")

add_test(NAME parser_knobs_allow_unknown_short_unknown_not_last_does_not_consume COMMAND parser_knobs_example unknown_ok -xv value)
set_tests_properties(parser_knobs_allow_unknown_short_unknown_not_last_does_not_consume PROPERTIES PASS_REGULAR_EXPRESSION "verbose=true args=\\[value\\]")

add_test(NAME parser_knobs_allow_unknown_short_unknown_last_consumes COMMAND parser_knobs_example unknown_ok -vx value)
set_tests_properties(parser_knobs_allow_unknown_short_unknown_last_consumes PROPERTIES PASS_REGULAR_EXPRESSION "verbose=true args=\\[\\]")

add_test(NAME parser_knobs_no_short_group COMMAND parser_knobs_example no_group -vq)
set_tests_properties(parser_knobs_no_short_group PROPERTIES PASS_REGULAR_EXPRESSION "unknown flag: -vq")

add_test(NAME parser_knobs_no_bool_negation COMMAND parser_knobs_example no_neg --no-verbose)
set_tests_properties(parser_knobs_no_bool_negation PROPERTIES PASS_REGULAR_EXPRESSION "unknown flag: --no-verbose")

add_executable(sorting_example examples/sorting_example.cpp)
target_link_libraries(sorting_example PRIVATE clasp)

add_test(NAME sorting_commands_default COMMAND sorting_example --help)
set_tests_properties(sorting_commands_default PROPERTIES PASS_REGULAR_EXPRESSION [==[Commands:
  a - A cmd
  b - B cmd]==])

add_test(NAME sorting_flags_default COMMAND sorting_example --help)
set_tests_properties(sorting_flags_default PROPERTIES PASS_REGULAR_EXPRESSION [==[Flags:
  --alpha - A flag
  -h, --help - Help for this command
  --zeta - Z flag]==])

add_executable(no_sorting_example examples/no_sorting_example.cpp)
target_link_libraries(no_sorting_example PRIVATE clasp)

add_test(NAME sorting_commands_disabled COMMAND no_sorting_example --help)
set_tests_properties(sorting_commands_disabled PROPERTIES PASS_REGULAR_EXPRESSION [==[Commands:
  b - B cmd
  a - A cmd]==])

add_test(NAME sorting_flags_disabled COMMAND no_sorting_example --help)
set_tests_properties(sorting_flags_disabled PROPERTIES PASS_REGULAR_EXPRESSION [==[Flags:
  --zeta - Z flag
  --alpha - A flag
  -h, --help - Help for this command]==])

add_executable(group_example examples/group_example.cpp)
target_link_libraries(group_example PRIVATE clasp)

add_test(NAME group_help COMMAND group_example --help)
set_tests_properties(group_help PROPERTIES PASS_REGULAR_EXPRESSION [==[Commands:
  zulu - Zulu cmd

Core Commands:
  alpha - Alpha cmd

Extra Commands:
  beta - Beta cmd]==])

add_executable(execute_example examples/execute_example.cpp)
target_link_libraries(execute_example PRIVATE clasp)

add_test(NAME execute_set_args COMMAND execute_example)
set_tests_properties(execute_set_args PROPERTIES PASS_REGULAR_EXPRESSION "from-execute")

add_executable(version_template_example examples/version_template_example.cpp)
target_link_libraries(version_template_example PRIVATE clasp)

add_test(NAME version_template_flag COMMAND version_template_example --version)
set_tests_properties(version_template_flag PROPERTIES PASS_REGULAR_EXPRESSION "app=1\\.2\\.3")

add_test(NAME version_template_subcommand_flag COMMAND version_template_example sub --version)
set_tests_properties(version_template_subcommand_flag PROPERTIES PASS_REGULAR_EXPRESSION "app sub=1\\.2\\.3")

add_executable(disable_flags_in_use_line_example examples/disable_flags_in_use_line_example.cpp)
target_link_libraries(disable_flags_in_use_line_example PRIVATE clasp)

add_test(NAME usage_line_no_flags_root COMMAND disable_flags_in_use_line_example --help)
set_tests_properties(usage_line_no_flags_root PROPERTIES PASS_REGULAR_EXPRESSION "Usage: app \\[command\\]" FAIL_REGULAR_EXPRESSION "\\[flags\\]")

add_test(NAME usage_line_flags_subcommand COMMAND disable_flags_in_use_line_example help sub)
set_tests_properties(usage_line_flags_subcommand PROPERTIES PASS_REGULAR_EXPRESSION "Usage: app sub \\[flags\\]" )

add_executable(flag_error_func_example examples/flag_error_func_example.cpp)
target_link_libraries(flag_error_func_example PRIVATE clasp)

add_test(NAME flag_error_func_unknown_flag COMMAND flag_error_func_example --nope)
set_tests_properties(flag_error_func_unknown_flag PROPERTIES PASS_REGULAR_EXPRESSION "Error: FLAGERR: unknown flag: --nope")

add_executable(help_command_rename_example examples/help_command_rename_example.cpp)
target_link_libraries(help_command_rename_example PRIVATE clasp)

add_test(NAME help_command_renamed COMMAND help_command_rename_example assist print)
set_tests_properties(help_command_renamed PROPERTIES PASS_REGULAR_EXPRESSION "Usage: app print")

add_test(NAME help_command_default_name_no_longer_works COMMAND help_command_rename_example help print)
set_tests_properties(help_command_default_name_no_longer_works PROPERTIES PASS_REGULAR_EXPRESSION "unknown command \"help\"")

add_executable(help_command_disabled_example examples/help_command_disabled_example.cpp)
target_link_libraries(help_command_disabled_example PRIVATE clasp)

add_test(NAME help_command_disabled COMMAND help_command_disabled_example help print)
set_tests_properties(help_command_disabled PROPERTIES PASS_REGULAR_EXPRESSION "unknown command \"help\"")

add_test(NAME help_flag_still_works_without_help_command COMMAND help_command_disabled_example --help)
set_tests_properties(help_flag_still_works_without_help_command PROPERTIES PASS_REGULAR_EXPRESSION "Usage: app \\[command\\]" )

add_executable(count_example examples/count_example.cpp)
target_link_libraries(count_example PRIVATE clasp)

add_test(NAME count_short_group COMMAND count_example -vvv)
set_tests_properties(count_short_group PROPERTIES PASS_REGULAR_EXPRESSION "count=3 occ=3 args=")

add_test(NAME count_long_repeat COMMAND count_example --verbose --verbose)
set_tests_properties(count_long_repeat PROPERTIES PASS_REGULAR_EXPRESSION "count=2 occ=2 args=")

add_test(NAME count_long_value_and_repeat COMMAND count_example --verbose=3 --verbose)
set_tests_properties(count_long_value_and_repeat PROPERTIES PASS_REGULAR_EXPRESSION "count=4 occ=2 args=")

add_test(NAME count_hex_value_and_repeat COMMAND count_example --verbose=0x3 --verbose)
set_tests_properties(count_hex_value_and_repeat PROPERTIES PASS_REGULAR_EXPRESSION "count=4 occ=2 args=")

add_test(NAME count_does_not_consume_next_arg COMMAND count_example --verbose 3)
set_tests_properties(count_does_not_consume_next_arg PROPERTIES PASS_REGULAR_EXPRESSION "count=1 occ=1 args=3")

add_executable(context_example examples/context_example.cpp)
target_link_libraries(context_example PRIVATE clasp)

add_test(NAME context_inherited COMMAND context_example show)
set_tests_properties(context_inherited PROPERTIES PASS_REGULAR_EXPRESSION "ctx=rootctx")

add_executable(custom_template_example examples/custom_template_example.cpp)
target_link_libraries(custom_template_example PRIVATE clasp)

add_test(NAME custom_help_template COMMAND custom_template_example --help)
set_tests_properties(custom_help_template PROPERTIES PASS_REGULAR_EXPRESSION "HELP>>Usage: app")

add_test(NAME custom_usage_template COMMAND custom_template_example --mystery)
set_tests_properties(custom_usage_template PROPERTIES PASS_REGULAR_EXPRESSION "USAGE>>Usage: app")

add_executable(normalize_example examples/normalize_example.cpp)
target_link_libraries(normalize_example PRIVATE clasp)

add_test(NAME normalize_flag_name COMMAND normalize_example --message_text hi)
set_tests_properties(normalize_flag_name PROPERTIES PASS_REGULAR_EXPRESSION "message=hi")

add_test(NAME normalize_bool_negation COMMAND normalize_example --no-do_thing)
set_tests_properties(normalize_bool_negation PROPERTIES PASS_REGULAR_EXPRESSION "doThingSeen=true")

add_executable(noopt_example examples/noopt_example.cpp)
target_link_libraries(noopt_example PRIVATE clasp)

add_test(NAME noopt_value_omitted COMMAND noopt_example --mode --other)
set_tests_properties(noopt_value_omitted PROPERTIES PASS_REGULAR_EXPRESSION "mode=auto.*other=true")

add_test(NAME noopt_value_provided COMMAND noopt_example --mode manual)
set_tests_properties(noopt_value_provided PROPERTIES PASS_REGULAR_EXPRESSION "mode=manual")

add_executable(repeat_example examples/repeat_example.cpp)
target_link_libraries(repeat_example PRIVATE clasp)

add_test(NAME repeat_last_wins COMMAND repeat_example --tag a --tag b)
set_tests_properties(repeat_last_wins PROPERTIES PASS_REGULAR_EXPRESSION "last=b")

add_test(NAME repeat_split_and_repeat COMMAND repeat_example --tag a,b --tag c)
set_tests_properties(repeat_split_and_repeat PROPERTIES PASS_REGULAR_EXPRESSION "tags=a\\|b\\|c")

add_test(NAME repeat_short_flag COMMAND repeat_example -t a -t b)
set_tests_properties(repeat_short_flag PROPERTIES PASS_REGULAR_EXPRESSION "tags=a\\|b")

add_executable(types_example examples/types_example.cpp)
target_link_libraries(types_example PRIVATE clasp)

add_test(NAME types_defaults COMMAND types_example)
set_tests_properties(types_defaults PROPERTIES PASS_REGULAR_EXPRESSION "i64=-1.*u64=0.*pi=3\\.14.*timeout_ms=1500")

add_test(NAME types_parse_values COMMAND types_example --i64 -5 --u64 7 --pi 2.5 --timeout 2s)
set_tests_properties(types_parse_values PROPERTIES PASS_REGULAR_EXPRESSION "i64=-5.*u64=7.*pi=2\\.5.*timeout_ms=2000")

add_test(NAME types_parse_hex_values COMMAND types_example --i64 0x10 --u64 0xff)
set_tests_properties(types_parse_hex_values PROPERTIES PASS_REGULAR_EXPRESSION "i64=16.*u64=255")

add_test(NAME types_parse_duration_compound COMMAND types_example --timeout 1s500ms)
set_tests_properties(types_parse_duration_compound PROPERTIES PASS_REGULAR_EXPRESSION "timeout_ms=1500")

add_test(NAME types_parse_duration_zero COMMAND types_example --timeout 0)
set_tests_properties(types_parse_duration_zero PROPERTIES PASS_REGULAR_EXPRESSION "timeout_ms=0")

add_test(NAME types_invalid_i64_trailing COMMAND types_example --i64 1,2)
set_tests_properties(types_invalid_i64_trailing PROPERTIES PASS_REGULAR_EXPRESSION [==[invalid argument "1,2" for "--i64"]==])

add_test(NAME types_invalid_u64_negative COMMAND types_example --u64 -1)
set_tests_properties(types_invalid_u64_negative PROPERTIES PASS_REGULAR_EXPRESSION [==[invalid argument "-1" for "--u64"]==])

add_test(NAME types_invalid_pi_trailing COMMAND types_example --pi 3.14x)
set_tests_properties(types_invalid_pi_trailing PROPERTIES PASS_REGULAR_EXPRESSION [==[invalid argument "3.14x" for "--pi"]==])

add_test(NAME types_invalid_timeout_unit COMMAND types_example --timeout 5q)
set_tests_properties(types_invalid_timeout_unit PROPERTIES PASS_REGULAR_EXPRESSION [==[invalid argument "5q" for "--timeout"]==])

add_executable(net_example examples/net_example.cpp)
target_link_libraries(net_example PRIVATE clasp)

add_test(NAME net_help_shows_types COMMAND net_example help show)
set_tests_properties(net_help_shows_types PROPERTIES PASS_REGULAR_EXPRESSION "(--ip ip.*--cidr cidr)|(--cidr cidr.*--ip ip)")

add_test(NAME net_ip_ipv6_canonical COMMAND net_example show --ip 2001:db8:0:0:0:0:2:1)
set_tests_properties(net_ip_ipv6_canonical PROPERTIES PASS_REGULAR_EXPRESSION "ip=2001:db8::2:1")

add_test(NAME net_cidr_v4_masks COMMAND net_example show --cidr 10.0.1.2/8)
set_tests_properties(net_cidr_v4_masks PROPERTIES PASS_REGULAR_EXPRESSION "cidr=10.0.0.0/8")

add_test(NAME net_cidr_v6_masks COMMAND net_example show --cidr 2001:db8::1/32)
set_tests_properties(net_cidr_v6_masks PROPERTIES PASS_REGULAR_EXPRESSION "cidr=2001:db8::/32")

add_test(NAME net_invalid_ip COMMAND net_example show --ip 999.0.0.1)
set_tests_properties(net_invalid_ip PROPERTIES PASS_REGULAR_EXPRESSION [==[invalid argument "999.0.0.1" for "--ip".*Usage: app show]==])

add_test(NAME net_invalid_cidr_prefix COMMAND net_example show --cidr 10.0.0.1/33)
set_tests_properties(net_invalid_cidr_prefix PROPERTIES PASS_REGULAR_EXPRESSION [==[invalid argument "10.0.0.1/33" for "--cidr".*Usage: app show]==])

add_executable(external_typed_example examples/external_typed_example.cpp)
target_link_libraries(external_typed_example PRIVATE clasp)

add_test(NAME external_typed_env_ok COMMAND external_typed_example)
set_tests_properties(external_typed_env_ok PROPERTIES
  ENVIRONMENT "APP_I64=-5;APP_U64=7;APP_TIMEOUT=2s"
  PASS_REGULAR_EXPRESSION "i64=-5.*u64=7.*timeout_ms=2000")

add_test(NAME external_typed_env_invalid_i64 COMMAND external_typed_example)
set_tests_properties(external_typed_env_invalid_i64 PROPERTIES
  ENVIRONMENT "APP_I64=1,2"
  PASS_REGULAR_EXPRESSION [==[invalid argument "1,2" for "--i64"]==])

add_test(NAME external_typed_env_invalid_timeout COMMAND external_typed_example)
set_tests_properties(external_typed_env_invalid_timeout PROPERTIES
  ENVIRONMENT "APP_TIMEOUT=5q"
  PASS_REGULAR_EXPRESSION [==[invalid argument "5q" for "--timeout"]==])

add_executable(bytes_example examples/bytes_example.cpp)
target_link_libraries(bytes_example PRIVATE clasp)

add_test(NAME bytes_help_shows_type COMMAND bytes_example --help)
set_tests_properties(bytes_help_shows_type PROPERTIES PASS_REGULAR_EXPRESSION "--limit bytes")

add_test(NAME bytes_default COMMAND bytes_example)
set_tests_properties(bytes_default PROPERTIES PASS_REGULAR_EXPRESSION "limit=0")

add_test(NAME bytes_parse_kb COMMAND bytes_example --limit 1KB)
set_tests_properties(bytes_parse_kb PROPERTIES PASS_REGULAR_EXPRESSION "limit=1024")

add_test(NAME bytes_parse_mib_fraction COMMAND bytes_example --limit 1.5MiB)
set_tests_properties(bytes_parse_mib_fraction PROPERTIES PASS_REGULAR_EXPRESSION "limit=1572864")

add_test(NAME bytes_parse_hex COMMAND bytes_example --limit 0x10)
set_tests_properties(bytes_parse_hex PROPERTIES PASS_REGULAR_EXPRESSION "limit=16")

add_test(NAME bytes_env_ok COMMAND bytes_example)
set_tests_properties(bytes_env_ok PROPERTIES ENVIRONMENT "APP_LIMIT=2G" PASS_REGULAR_EXPRESSION "limit=2147483648")

add_test(NAME bytes_invalid_unit COMMAND bytes_example --limit 1XB)
set_tests_properties(bytes_invalid_unit PROPERTIES PASS_REGULAR_EXPRESSION [==[invalid argument "1XB" for "--limit"]==])

add_test(NAME bytes_invalid_negative COMMAND bytes_example --limit -1)
set_tests_properties(bytes_invalid_negative PROPERTIES PASS_REGULAR_EXPRESSION [==[invalid argument "-1" for "--limit"]==])

add_executable(directive_example examples/directive_example.cpp)
target_link_libraries(directive_example PRIVATE clasp)

add_test(NAME completion_directive_override COMMAND directive_example __completeNoDesc fruit a)
set_tests_properties(completion_directive_override PROPERTIES PASS_REGULAR_EXPRESSION ":8")

add_executable(completion_naming_example examples/completion_naming_example.cpp)
target_link_libraries(completion_naming_example PRIVATE clasp)

add_test(NAME completion_custom_command_name COMMAND completion_naming_example --help)
set_tests_properties(completion_custom_command_name PROPERTIES PASS_REGULAR_EXPRESSION "comp - Generate shell completion scripts")

add_test(NAME completion_custom_powershell_script_internal_name COMMAND completion_naming_example comp powershell)
set_tests_properties(completion_custom_powershell_script_internal_name PROPERTIES PASS_REGULAR_EXPRESSION "__cnd")

add_test(NAME completion_custom_script_internal_name COMMAND completion_naming_example comp bash)
set_tests_properties(completion_custom_script_internal_name PROPERTIES PASS_REGULAR_EXPRESSION "__cnd")

add_test(NAME completion_custom_internal_name COMMAND completion_naming_example __cnd p)
set_tests_properties(completion_custom_internal_name PROPERTIES PASS_REGULAR_EXPRESSION "paint")

add_test(NAME completion_custom_fish_script_internal_name COMMAND completion_naming_example comp fish)
set_tests_properties(completion_custom_fish_script_internal_name PROPERTIES PASS_REGULAR_EXPRESSION "__c")

add_executable(pflag_types_example examples/pflag_types_example.cpp)
target_link_libraries(pflag_types_example PRIVATE clasp)

add_test(NAME pflag_string_slice_splits COMMAND pflag_types_example show --names a,b --names c)
set_tests_properties(pflag_string_slice_splits PROPERTIES PASS_REGULAR_EXPRESSION "names=a\\|b\\|c")

add_test(NAME pflag_string_array_no_split COMMAND pflag_types_example show --tags a,b --tags c)
set_tests_properties(pflag_string_array_no_split PROPERTIES PASS_REGULAR_EXPRESSION "tags=a,b\\|c")

add_test(NAME pflag_string_map_parses COMMAND pflag_types_example show --labels a=1,b=2 --labels c=3)
set_tests_properties(pflag_string_map_parses PROPERTIES PASS_REGULAR_EXPRESSION "labels=a=1\\|b=2\\|c=3")

add_test(NAME pflag_bool_slice_splits COMMAND pflag_types_example show --gates true,false --gates yes)
set_tests_properties(pflag_bool_slice_splits PROPERTIES PASS_REGULAR_EXPRESSION "gates=true\\|false\\|true")

add_test(NAME pflag_int_slice_splits COMMAND pflag_types_example show --ports 1,2 --ports 3)
set_tests_properties(pflag_int_slice_splits PROPERTIES PASS_REGULAR_EXPRESSION "ports=1\\|2\\|3")

add_test(NAME pflag_int_array COMMAND pflag_types_example show --nums 1 --nums 2)
set_tests_properties(pflag_int_array PROPERTIES PASS_REGULAR_EXPRESSION "nums=1\\|2")

add_test(NAME pflag_map_int64 COMMAND pflag_types_example show --big a=1,b=2 --big c=3)
set_tests_properties(pflag_map_int64 PROPERTIES PASS_REGULAR_EXPRESSION "big=a=1\\|b=2\\|c=3")

add_test(NAME pflag_map_bool COMMAND pflag_types_example show --toggles a=true --toggles b=false,c=yes)
set_tests_properties(pflag_map_bool PROPERTIES PASS_REGULAR_EXPRESSION "toggles=a=true\\|b=false\\|c=true")

add_executable(custom_value_example examples/custom_value_example.cpp)
target_link_libraries(custom_value_example PRIVATE clasp)

add_test(NAME custom_value_default COMMAND custom_value_example show)
set_tests_properties(custom_value_default PROPERTIES PASS_REGULAR_EXPRESSION "info")

add_test(NAME custom_value_ok COMMAND custom_value_example show --level debug)
set_tests_properties(custom_value_ok PROPERTIES PASS_REGULAR_EXPRESSION "debug")

add_test(NAME custom_value_config COMMAND custom_value_example show --config ${CMAKE_CURRENT_SOURCE_DIR}/examples/custom_value.env)
set_tests_properties(custom_value_config PROPERTIES PASS_REGULAR_EXPRESSION "warn")

add_test(NAME custom_value_env_overrides_config COMMAND custom_value_example show --config ${CMAKE_CURRENT_SOURCE_DIR}/examples/custom_value.env)
set_tests_properties(custom_value_env_overrides_config PROPERTIES ENVIRONMENT "APP_LEVEL=error" PASS_REGULAR_EXPRESSION "error")

add_test(NAME custom_value_repeat_last_wins COMMAND custom_value_example show --level debug --level warn)
set_tests_properties(custom_value_repeat_last_wins PROPERTIES PASS_REGULAR_EXPRESSION "warn")

add_test(NAME custom_value_invalid COMMAND custom_value_example show --level loud)
set_tests_properties(custom_value_invalid PROPERTIES PASS_REGULAR_EXPRESSION "invalid level")

add_test(NAME custom_value_help_shows_type COMMAND custom_value_example help show)
set_tests_properties(custom_value_help_shows_type PROPERTIES PASS_REGULAR_EXPRESSION "--level level")

add_executable(silence_errors_example examples/silence_errors_example.cpp)
target_link_libraries(silence_errors_example PRIVATE clasp)

add_test(NAME silence_errors_unknown_command COMMAND silence_errors_example nope)
set_tests_properties(silence_errors_unknown_command PROPERTIES PASS_REGULAR_EXPRESSION ".*" FAIL_REGULAR_EXPRESSION "Error:|Run 'app")

add_executable(map_example examples/map_example.cpp)
target_link_libraries(map_example PRIVATE clasp)

add_test(NAME map_single COMMAND map_example --label a=1,b=2)
set_tests_properties(map_single PROPERTIES PASS_REGULAR_EXPRESSION "labels=a=1,b=2")

add_test(NAME map_repeat_overrides COMMAND map_example --label a=1 --label a=9)
set_tests_properties(map_repeat_overrides PROPERTIES PASS_REGULAR_EXPRESSION "labels=a=9")
