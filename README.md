# Clasp

[![CI](https://github.com/cuihairu/clasp/actions/workflows/ci.yml/badge.svg)](https://github.com/cuihairu/clasp/actions/workflows/ci.yml)
[![Version](https://img.shields.io/github/v/tag/cuihairu/clasp?sort=semver)](https://github.com/cuihairu/clasp/tags)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/github/license/cuihairu/clasp)](LICENSE)
[![Codecov](https://codecov.io/gh/cuihairu/clasp/branch/main/graph/badge.svg)](https://codecov.io/gh/cuihairu/clasp)

**Clasp** is a modern C++ library designed for building powerful command-line applications. Inspired by Go's Cobra library, Clasp offers a straightforward API to define and organize commands, parse flags, and manage CLI application lifecycles.

## Features

- **Command and Subcommand Handling**: Easily define and manage commands with subcommands and associated actions.
- **Argument Parsing**: Supports both positional and named arguments with type safety.
- **Flag Management**: Persistent/local flags, required/hidden/deprecated, groups, repeated flags, pflag-style optional values (`NoOptDefVal`), and extra helpers like bytes/count/IP/CIDR/IPNet/IPMask/URL.
- **Optional Colored Output**: Opt-in ANSI color for built-in help/usage/errors, with built-in themes (`vscode`, `sublime`, `iterm2`) and `--color=auto|always|never`.
- **Cobra-Like Ergonomics**: Hooks, aliases, suggestions, `TraverseChildren`, sorted help output, examples, and custom help/usage/version templates.
- **Help and Usage Generation**: Automatically generate help text and usage instructions based on defined commands and flags.
- **Shell Completion**: bash/zsh/fish/powershell completion generation + `__complete` directives and configurable completion command names.
- **Config Integration**: env binding and config file merge (`.env`-style key=value, `.ini`/`.cfg` with `[section]` mapping, and basic nested-object flatten for `.json`/`.toml`/`.yaml`). Unknown extensions are rejected.
- **Extensibility**: Flexible enough to be extended for more complex CLI needs.

Flag parsing is intentionally pflag-like: `--k=v`, `-k=v`, short grouping (`-abc`), bool negation (`--no-foo`), repeated occurrences, and optional values are all supported. When command traversal is enabled, Clasp keeps subcommand discovery separate from optional flag values, so a token that names a subcommand is not consumed as the value for a `NoOptDefVal` flag.

## Installation

### Prerequisites

- **C++17**: Clasp targets C++17 for broader compiler compatibility.

### Build from Source

```bash
git clone https://github.com/cuihairu/clasp.git
cd clasp
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build
# or: cmake --install build --prefix /your/prefix
```

To build/install only the library (no examples/CTest), configure with `-DCLASP_BUILD_EXAMPLES=OFF`.

## Getting Started

### Basic Example

```cpp
#include "clasp/clasp.hpp"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    clasp::Command rootCmd("app", "A brief description of your application");

    clasp::Command printCmd("print", "Prints a message to the console");
    printCmd
        .withFlag("--message", "-m", "message", "Message to print", std::string("Hello, World!"))
        .action([](clasp::Command& /*cmd*/, const clasp::Parser& parser, const std::vector<std::string>& /*args*/) {
            const auto message = parser.getFlag<std::string>("--message", "Hello, World!");
            std::cout << message << "\n";
            return 0;
        });

    rootCmd.addCommand(std::move(printCmd));
    return rootCmd.run(argc, argv);
}
```

### Running the Example

```bash
./app print --message "Hello, Clasp!"
```

### Output:

```
Hello, Clasp!
```

## Documentation

- `EXAMPLES.md`: index of all runnable examples and what they demonstrate.
- `COMPAT.md`: what “Cobra-like” means for this project.
- `CHANGELOG.md`: notable changes and release notes.
- Public headers live under `include/clasp/` (`clasp/clasp.hpp` includes the main API).
- `docs/`: VuePress site sources (optional).

## Behavior Notes

- Flag precedence is `CLI > env > config file > default`.
- `configFile("path")` hardwires a config path; `configFileFlag("config")` lets a flag such as `--config` supply it.
- Supported config formats are `.env`, `.ini`, `.cfg`, `.json`, `.toml`, and `.yaml`/`.yml`. Unknown extensions are rejected.
- Completion supports generated shell scripts plus dynamic `__complete` / `__completeNoDesc` commands.
- File and directory completion can be attached to both local and persistent flags with `markFlagFilename(...)` and `markPersistentFlagFilename(...)`, or `markFlagDirname()` and `markPersistentFlagDirname()`.
- On multi-config generators such as Visual Studio, run tests with an explicit configuration, for example `ctest --test-dir build -C Debug --output-on-failure`.

## Coverage

The test suite is broad, but coverage report generation depends on toolchain support:

- GCC/Clang: configure with `-DCLASP_ENABLE_COVERAGE=ON` and use your usual `gcov`/`lcov` flow.
- Visual Studio/MSVC: the library and tests build and run, but `CLASP_ENABLE_COVERAGE` does not instrument MSVC builds. Use Visual Studio code coverage tooling if you need a Windows coverage report.

### VuePress Docs (Optional)

```bash
cd docs
npm install
npm run dev
```

## Versioning

Clasp follows SemVer. The version in `CMakeLists.txt` is kept consistent with the `CLASP_VERSION_*` macros in `include/clasp/clasp.hpp`.

## Using With CMake

After installing (or setting `CMAKE_PREFIX_PATH` to your install prefix), consume Clasp via:

```cmake
find_package(clasp CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE clasp::clasp)
```

## Contributing

Issues and PRs are welcome.

## License

Clasp is licensed under the Apache License 2.0. See [LICENSE](LICENSE) for more details.
