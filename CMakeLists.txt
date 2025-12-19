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
