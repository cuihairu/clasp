# Example executables for Clasp. Included from the root CMakeLists.txt when
# CLASP_BUILD_EXAMPLES is ON. Each target links clasp privately; the CTest
# behavior assertions live in tests.cmake and must be included after this file.


add_executable(basic_example examples/basic_example.cpp)
target_link_libraries(basic_example PRIVATE clasp)


add_executable(args_example examples/args_example.cpp)
target_link_libraries(args_example PRIVATE clasp)


add_executable(hooks_example examples/hooks_example.cpp)
target_link_libraries(hooks_example PRIVATE clasp)


add_executable(required_example examples/required_example.cpp)
target_link_libraries(required_example PRIVATE clasp)

add_executable(docs_example examples/docs_example.cpp)
target_link_libraries(docs_example PRIVATE clasp)


add_executable(completion_example examples/completion_example.cpp)
target_link_libraries(completion_example PRIVATE clasp)


add_executable(file_completion_example examples/file_completion_example.cpp)
target_link_libraries(file_completion_example PRIVATE clasp)


add_executable(keep_order_example examples/keep_order_example.cpp)
target_link_libraries(keep_order_example PRIVATE clasp)


add_executable(config_example examples/config_example.cpp)
target_link_libraries(config_example PRIVATE clasp)


add_executable(config_json_example examples/config_json_example.cpp)
target_link_libraries(config_json_example PRIVATE clasp)


add_executable(config_toml_example examples/config_toml_example.cpp)
target_link_libraries(config_toml_example PRIVATE clasp)


add_executable(config_yaml_example examples/config_yaml_example.cpp)
target_link_libraries(config_yaml_example PRIVATE clasp)


add_executable(config_list_example examples/config_list_example.cpp)
target_link_libraries(config_list_example PRIVATE clasp)


add_executable(config_ini_example examples/config_ini_example.cpp)
target_link_libraries(config_ini_example PRIVATE clasp)


add_executable(traverse_example examples/traverse_example.cpp)
target_link_libraries(traverse_example PRIVATE clasp)


add_executable(flag_groups_example examples/flag_groups_example.cpp)
target_link_libraries(flag_groups_example PRIVATE clasp)


add_executable(no_suggest_example examples/no_suggest_example.cpp)
target_link_libraries(no_suggest_example PRIVATE clasp)


add_executable(dynamic_completion_example examples/dynamic_completion_example.cpp)
target_link_libraries(dynamic_completion_example PRIVATE clasp)


add_executable(parser_knobs_example examples/parser_knobs_example.cpp)
target_link_libraries(parser_knobs_example PRIVATE clasp)


add_executable(sorting_example examples/sorting_example.cpp)
target_link_libraries(sorting_example PRIVATE clasp)


add_executable(no_sorting_example examples/no_sorting_example.cpp)
target_link_libraries(no_sorting_example PRIVATE clasp)


add_executable(group_example examples/group_example.cpp)
target_link_libraries(group_example PRIVATE clasp)


add_executable(execute_example examples/execute_example.cpp)
target_link_libraries(execute_example PRIVATE clasp)


add_executable(version_template_example examples/version_template_example.cpp)
target_link_libraries(version_template_example PRIVATE clasp)


add_executable(disable_flags_in_use_line_example examples/disable_flags_in_use_line_example.cpp)
target_link_libraries(disable_flags_in_use_line_example PRIVATE clasp)


add_executable(flag_error_func_example examples/flag_error_func_example.cpp)
target_link_libraries(flag_error_func_example PRIVATE clasp)


add_executable(help_command_rename_example examples/help_command_rename_example.cpp)
target_link_libraries(help_command_rename_example PRIVATE clasp)


add_executable(help_command_disabled_example examples/help_command_disabled_example.cpp)
target_link_libraries(help_command_disabled_example PRIVATE clasp)


add_executable(count_example examples/count_example.cpp)
target_link_libraries(count_example PRIVATE clasp)


add_executable(context_example examples/context_example.cpp)
target_link_libraries(context_example PRIVATE clasp)


add_executable(custom_template_example examples/custom_template_example.cpp)
target_link_libraries(custom_template_example PRIVATE clasp)


add_executable(normalize_example examples/normalize_example.cpp)
target_link_libraries(normalize_example PRIVATE clasp)


add_executable(noopt_example examples/noopt_example.cpp)
target_link_libraries(noopt_example PRIVATE clasp)


add_executable(repeat_example examples/repeat_example.cpp)
target_link_libraries(repeat_example PRIVATE clasp)


add_executable(types_example examples/types_example.cpp)
target_link_libraries(types_example PRIVATE clasp)


add_executable(net_example examples/net_example.cpp)
target_link_libraries(net_example PRIVATE clasp)


add_executable(net_extra_example examples/net_extra_example.cpp)
target_link_libraries(net_extra_example PRIVATE clasp)


add_executable(url_example examples/url_example.cpp)
target_link_libraries(url_example PRIVATE clasp)


add_executable(flag_convert_example examples/flag_convert_example.cpp)
target_link_libraries(flag_convert_example PRIVATE clasp)


add_executable(external_typed_example examples/external_typed_example.cpp)
target_link_libraries(external_typed_example PRIVATE clasp)


add_executable(bytes_example examples/bytes_example.cpp)
target_link_libraries(bytes_example PRIVATE clasp)


add_executable(directive_example examples/directive_example.cpp)
target_link_libraries(directive_example PRIVATE clasp)


add_executable(completion_naming_example examples/completion_naming_example.cpp)
target_link_libraries(completion_naming_example PRIVATE clasp)


add_executable(pflag_types_example examples/pflag_types_example.cpp)
target_link_libraries(pflag_types_example PRIVATE clasp)


add_executable(pflag_types2_example examples/pflag_types2_example.cpp)
target_link_libraries(pflag_types2_example PRIVATE clasp)


add_executable(utils_suggest_example examples/utils_suggest_example.cpp)
target_link_libraries(utils_suggest_example PRIVATE clasp)


add_executable(traverse_completion_example examples/traverse_completion_example.cpp)
target_link_libraries(traverse_completion_example PRIVATE clasp)


add_executable(deprecated_example examples/deprecated_example.cpp)
target_link_libraries(deprecated_example PRIVATE clasp)


add_executable(custom_value_example examples/custom_value_example.cpp)
target_link_libraries(custom_value_example PRIVATE clasp)


add_executable(float_flag_example examples/float_flag_example.cpp)
target_link_libraries(float_flag_example PRIVATE clasp)


add_executable(api_test tests/api.cpp)
target_link_libraries(api_test PRIVATE clasp)


add_executable(command_test tests/command.cpp)
target_link_libraries(command_test PRIVATE clasp)


add_executable(completion_test tests/completion.cpp)
target_link_libraries(completion_test PRIVATE clasp)


# --- Execute coverage tests ---

add_executable(execute_test tests/execute.cpp)
target_link_libraries(execute_test PRIVATE clasp)


# --- Parser coverage tests ---

add_executable(parser_test tests/parser.cpp)
target_link_libraries(parser_test PRIVATE clasp)


# --- Template coverage tests ---

add_executable(template_test tests/template.cpp)
target_link_libraries(template_test PRIVATE clasp)


# --- Utils coverage tests ---

add_executable(utils_test tests/utils.cpp)
target_link_libraries(utils_test PRIVATE clasp)


# --- Parser advanced coverage tests ---

add_executable(parser_advanced_test tests/parser_advanced.cpp)
target_link_libraries(parser_advanced_test PRIVATE clasp)


# --- Command advanced coverage tests ---

add_executable(command_advanced_test tests/command_advanced.cpp)
target_link_libraries(command_advanced_test PRIVATE clasp)


# --- Special flag types coverage tests ---

add_executable(special_flags_test tests/special_flags.cpp)
target_link_libraries(special_flags_test PRIVATE clasp)


# --- Parser misc coverage tests ---

add_executable(parser_misc_test tests/parser_misc.cpp)
target_link_libraries(parser_misc_test PRIVATE clasp)


# --- Command misc coverage tests ---

add_executable(command_misc_test tests/command_misc.cpp)
target_link_libraries(command_misc_test PRIVATE clasp)


# --- Value parse coverage tests ---

add_executable(value_parse_test tests/value_parse.cpp)
target_link_libraries(value_parse_test PRIVATE clasp)


# --- Value interface coverage tests ---

add_executable(value_test tests/value.cpp)
target_link_libraries(value_test PRIVATE clasp)


# --- Parser gap coverage tests ---

add_executable(parser_gaps_test tests/parser_gaps.cpp)
target_link_libraries(parser_gaps_test PRIVATE clasp)


# --- Command gap coverage tests ---

add_executable(command_gaps_test tests/command_gaps.cpp)
target_link_libraries(command_gaps_test PRIVATE clasp)


# --- Final coverage gap tests ---

add_executable(coverage_final_test tests/coverage_final.cpp)
target_link_libraries(coverage_final_test PRIVATE clasp)


# --- Color coverage tests ---

add_executable(color_test tests/color.cpp)
target_link_libraries(color_test PRIVATE clasp)


# --- Parser edge coverage tests ---

add_executable(parser_edge_test tests/parser_edge.cpp)
target_link_libraries(parser_edge_test PRIVATE clasp)


# --- Command edge coverage tests ---

add_executable(command_edge_test tests/command_edge.cpp)
target_link_libraries(command_edge_test PRIVATE clasp)


# --- Command coverage tests for simplified flag methods ---

add_executable(command_coverage_test tests/command_coverage.cpp)
target_link_libraries(command_coverage_test PRIVATE clasp)


# --- Run coverage tests ---

add_executable(run_coverage_test tests/run_coverage.cpp)
target_link_libraries(run_coverage_test PRIVATE clasp)


# --- Hook coverage tests ---

add_executable(hook_test tests/hook.cpp)
target_link_libraries(hook_test PRIVATE clasp)


add_executable(parser_external_example examples/parser_external_example.cpp)
target_link_libraries(parser_external_example PRIVATE clasp)


add_executable(persistent_completion_test tests/persistent_completion.cpp)
target_link_libraries(persistent_completion_test PRIVATE clasp)


add_executable(hook_error_example examples/hook_error_example.cpp)
target_link_libraries(hook_error_example PRIVATE clasp)


add_executable(silence_errors_example examples/silence_errors_example.cpp)
target_link_libraries(silence_errors_example PRIVATE clasp)


add_executable(map_example examples/map_example.cpp)
target_link_libraries(map_example PRIVATE clasp)


# --- Parser extra coverage tests ---

add_executable(parser_extra_coverage_test tests/parser_extra_coverage.cpp)
target_link_libraries(parser_extra_coverage_test PRIVATE clasp)


# --- Branch coverage: config file parsing internals ---

add_executable(config_parse_coverage_test tests/config_parse_coverage.cpp)
target_link_libraries(config_parse_coverage_test PRIVATE clasp)


# --- Branch coverage: completion internals ---

add_executable(completion_coverage_test
    tests/completion_coverage.cpp
    tests/completion_protocol.cpp
    tests/completion_scripts.cpp
    tests/completion_e2e.cpp)
target_link_libraries(completion_coverage_test PRIVATE clasp)


# --- Branch coverage: help/color rendering and setter fallbacks ---

add_executable(command_render_coverage_test tests/command_render_coverage.cpp)
target_link_libraries(command_render_coverage_test PRIVATE clasp)


# --- Branch coverage: remaining parser.hpp / value_parse.hpp branches ---

add_executable(branch_parser_test tests/branch_parser.cpp)
target_link_libraries(branch_parser_test PRIVATE clasp)


# --- Annotation truthiness: help type names + parser registration ---

add_executable(annotation_values_test tests/annotation_values.cpp)
target_link_libraries(annotation_values_test PRIVATE clasp)


# --- Must Support: canonical Cobra-like strings locked byte-for-byte ---
add_executable(cobra_strings_example examples/cobra_strings_example.cpp)
target_link_libraries(cobra_strings_example PRIVATE clasp)


# --- Must Support: pflag edge interactions pinned by behavior ---
add_executable(pflag_edge_example examples/pflag_edge_example.cpp)
target_link_libraries(pflag_edge_example PRIVATE clasp)

