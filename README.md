# Clasp

**Clasp** is a modern C++ library designed for building powerful command-line applications. Inspired by Go's Cobra library, Clasp offers a straightforward API to define and organize commands, parse flags, and manage CLI application lifecycles.

## Features

- **Command and Subcommand Handling**: Easily define and manage commands with subcommands and associated actions.
- **Argument Parsing**: Supports both positional and named arguments with type safety.
- **Flag Management**: Define and parse command-line flags effortlessly.
- **Help and Usage Generation**: Automatically generate help text and usage instructions based on defined commands and flags.
- **Extensibility**: Flexible enough to be extended for more complex CLI needs.

## Installation

### Prerequisites

- **C++20**: Clasp utilizes modern C++ features, so a C++20 compliant compiler is required.

### Build from Source

```bash
git clone https://github.com/yourusername/clasp.git
cd clasp
mkdir build && cd build
cmake ..
make
sudo make install
```

## Getting Started

### Basic Example

```cpp
#include <clasp/clasp.hpp>

int main(int argc, char** argv) {
    auto rootCmd = Clasp::Command("app", "A brief description of your application");

    auto printCmd = Clasp::Command("print", "Prints a message to the console")
        .withFlag<std::string>("--message", "-m", "Message to print", "Hello, World!")
        .action([](auto& args) {
            std::string message = args.getFlag<std::string>("--message");
            std::cout << message << std::endl;
        });

    rootCmd.addCommand(printCmd);
    rootCmd.run(argc, argv);

    return 0;
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

Comprehensive documentation and API references are available at [Clasp Documentation](https://yourdocurl.com).

## Contributing

We welcome contributions! Please see our [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

Clasp is licensed under the MIT License. See [LICENSE](LICENSE) for more details.
