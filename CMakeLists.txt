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

add_test(NAME basic_unknown COMMAND basic_example nope)
set_tests_properties(basic_unknown PROPERTIES PASS_REGULAR_EXPRESSION "unknown command")

add_test(NAME basic_missing_flag_value COMMAND basic_example print --message)
set_tests_properties(basic_missing_flag_value PROPERTIES PASS_REGULAR_EXPRESSION "flag needs an argument")

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

add_test(NAME basic_alias COMMAND basic_example p --message hi)
set_tests_properties(basic_alias PROPERTIES PASS_REGULAR_EXPRESSION "hi")

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
set_tests_properties(required_missing PROPERTIES PASS_REGULAR_EXPRESSION "required flag not set")

add_test(NAME required_ok COMMAND required_example do --name bob)
set_tests_properties(required_ok PROPERTIES PASS_REGULAR_EXPRESSION "bob")

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

add_test(NAME completion_zsh COMMAND completion_example completion zsh)
set_tests_properties(completion_zsh PROPERTIES PASS_REGULAR_EXPRESSION "bashcompinit")

add_test(NAME completion_fish COMMAND completion_example completion fish)
set_tests_properties(completion_fish PROPERTIES PASS_REGULAR_EXPRESSION "complete -c app")

add_test(NAME completion_powershell COMMAND completion_example completion powershell)
set_tests_properties(completion_powershell PROPERTIES PASS_REGULAR_EXPRESSION "Register-ArgumentCompleter")

add_executable(config_example examples/config_example.cpp)
target_link_libraries(config_example PRIVATE clasp)

add_test(NAME config_from_file COMMAND config_example do --config ${CMAKE_CURRENT_SOURCE_DIR}/examples/config.env)
set_tests_properties(config_from_file PROPERTIES PASS_REGULAR_EXPRESSION "fromfile")

add_test(NAME config_env_overrides COMMAND config_example do --config ${CMAKE_CURRENT_SOURCE_DIR}/examples/config.env)
set_tests_properties(config_env_overrides PROPERTIES ENVIRONMENT "APP_MESSAGE=fromenv" PASS_REGULAR_EXPRESSION "fromenv")

add_test(NAME config_flag_overrides_env COMMAND config_example do --config ${CMAKE_CURRENT_SOURCE_DIR}/examples/config.env --message fromcli)
set_tests_properties(config_flag_overrides_env PROPERTIES ENVIRONMENT "APP_MESSAGE=fromenv" PASS_REGULAR_EXPRESSION "fromcli")

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
