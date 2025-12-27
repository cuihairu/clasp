# Clasp

[![CI](https://github.com/cuihairu/clasp/actions/workflows/ci.yml/badge.svg)](https://github.com/cuihairu/clasp/actions/workflows/ci.yml)
[![Version](https://img.shields.io/github/v/tag/cuihairu/clasp?sort=semver)](https://github.com/cuihairu/clasp/tags)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/w/cpp/17)
[![Codecov](https://codecov.io/gh/cuihairu/clasp/branch/main/graph/badge.svg)](https://codecov.io/gh/cuihairu/clasp)

**Clasp** is a modern C++ library designed for building powerful command-line applications. Inspired by Go's Cobra library, Clasp offers a straightforward API to define and organize commands, parse flags, and manage CLI application lifecycles.

## Features

- **Command and Subcommand Handling**: Easily define and manage commands with subcommands and associated actions.
- **Argument Parsing**: Supports both positional and named arguments with type safety.
- **Flag Management**: Persistent/local flags, required/hidden/deprecated, groups, repeated flags, optional values (NoOptDefVal), and extra helpers like bytes/count/IP/CIDR/IPNet/IPMask/URL.
- **Cobra-Like Ergonomics**: Hooks, aliases, suggestions, `TraverseChildren`, sorted help output, examples, and custom help/usage/version templates.
- **Help and Usage Generation**: Automatically generate help text and usage instructions based on defined commands and flags.
- **Shell Completion**: bash/zsh/fish/powershell completion generation + `__complete` directives and configurable completion command names.
- **Config Integration**: env binding and config file merge (supports `.env`-style, `.json`, `.toml`, `.yaml`).
- **Extensibility**: Flexible enough to be extended for more complex CLI needs.

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

Clasp is licensed under the MIT License. See [LICENSE](LICENSE) for more details.
