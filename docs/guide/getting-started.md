# Getting Started

## Building and Testing

```bash
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build
```

To build only the library (without examples/CTest):

```bash
cmake -S . -B build -DCLASP_BUILD_EXAMPLES=OFF
cmake --build build --parallel
cmake --install build
```

## Minimal Example

```cpp
#include "clasp/clasp.hpp"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  clasp::Command root("app", "A brief description of your application");

  clasp::Command print("print", "Prints a message");
  print.withFlag("--message", "-m", "message", "Message to print", std::string("Hello"))
       .action([](clasp::Command&, const clasp::Parser& p, const std::vector<std::string>&) {
         std::cout << p.getFlag<std::string>("--message", "Hello") << "\n";
         return 0;
       });

  root.addCommand(std::move(print));
  return root.run(argc, argv);
}
```

## CMake Integration

After installation:

```cmake
find_package(clasp CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE clasp::clasp)
```
