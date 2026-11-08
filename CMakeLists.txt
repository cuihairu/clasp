cmake_minimum_required(VERSION 3.14)
project(clasp VERSION 0.1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/include/clasp/clasp.hpp CLASP_HPP_CONTENTS)
string(REGEX MATCH "#define[ \t]+CLASP_VERSION_MAJOR[ \t]+([0-9]+)" _ ${CLASP_HPP_CONTENTS})
set(CLASP_HEADER_VERSION_MAJOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "#define[ \t]+CLASP_VERSION_MINOR[ \t]+([0-9]+)" _ ${CLASP_HPP_CONTENTS})
set(CLASP_HEADER_VERSION_MINOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "#define[ \t]+CLASP_VERSION_PATCH[ \t]+([0-9]+)" _ ${CLASP_HPP_CONTENTS})
set(CLASP_HEADER_VERSION_PATCH "${CMAKE_MATCH_1}")
set(CLASP_HEADER_VERSION "${CLASP_HEADER_VERSION_MAJOR}.${CLASP_HEADER_VERSION_MINOR}.${CLASP_HEADER_VERSION_PATCH}")
if(NOT CLASP_HEADER_VERSION STREQUAL PROJECT_VERSION)
  message(FATAL_ERROR "Version mismatch: project(clasp VERSION ${PROJECT_VERSION}) but include/clasp/clasp.hpp defines ${CLASP_HEADER_VERSION}")
endif()

set(CLASP_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(CLASP_TOP_LEVEL ON)
endif()
option(CLASP_BUILD_EXAMPLES "Build Clasp examples and CTest suite" ${CLASP_TOP_LEVEL})
option(CLASP_ENABLE_COVERAGE "Enable code coverage flags (GCC/Clang only)" OFF)

if(CLASP_ENABLE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O0 -g --coverage)
    add_link_options(--coverage)
  else()
    message(WARNING "CLASP_ENABLE_COVERAGE is only supported with GCC/Clang")
  endif()
endif()

add_library(clasp
        src/clasp.cpp
        src/command.cpp
        src/color.cpp
        src/flag.cpp
)

target_include_directories(clasp
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(clasp PUBLIC cxx_std_17)

set_target_properties(clasp PROPERTIES
  EXPORT_NAME clasp
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

add_library(clasp::clasp ALIAS clasp)

install(TARGETS clasp
  EXPORT claspTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT claspTargets
  NAMESPACE clasp::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clasp
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/claspConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/claspConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clasp
)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/claspConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/claspConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/claspConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clasp
)

if(CLASP_BUILD_EXAMPLES)
  include(CTest)
  enable_testing()

include(cmake/examples.cmake)
include(cmake/tests.cmake)
endif()
