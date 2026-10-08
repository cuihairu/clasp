# Clasp

[![CI](https://github.com/cuihairu/clasp/actions/workflows/ci.yml/badge.svg)](https://github.com/cuihairu/clasp/actions/workflows/ci.yml)
[![Version](https://img.shields.io/github/v/tag/cuihairu/clasp?sort=semver)](https://github.com/cuihairu/clasp/tags)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Codecov](https://codecov.io/gh/cuihairu/clasp/branch/main/graph/badge.svg)](https://codecov.io/gh/cuihairu/clasp)

[English](README.md) | 简体中文

**Clasp** 是一个用于构建强大命令行应用的现代 C++ 库。其设计灵感来自 Go 的 Cobra 库,Clasp 提供了简洁直观的 API 来定义和组织命令、解析 flag,以及管理 CLI 应用的完整生命周期。

## 特性

- **命令与子命令**:轻松定义和管理命令、子命令及其关联的动作。
- **参数解析**:同时支持位置参数和命名参数,并具备类型安全。
- **Flag 管理**:持久/局部 flag,支持 required/hidden/deprecated、分组、重复 flag、pflag 风格的可选值(`NoOptDefVal`),以及 bytes/count/IP/CIDR/IPNet/IPMask/URL 等额外辅助类型。
- **可选的彩色输出**:为内置的 help/usage/错误信息提供可选的 ANSI 颜色,内置主题(`vscode`、`sublime`、`iterm2`),支持 `--color=auto|always|never`。
- **类 Cobra 的易用性**:Hook、别名、命令建议、`TraverseChildren`、排序的 help 输出、示例,以及自定义 help/usage/version 模板。
- **Help 与 Usage 生成**:根据已定义的命令和 flag 自动生成帮助文本和用法说明。
- **Shell 补全**:生成 bash/zsh/fish/powershell 补全脚本,支持 `__complete` 指令和可配置的补全命令名。
- **配置集成**:支持环境变量绑定与配置文件合并(`.env` 风格的 key=value、带 `[section]` 映射的 `.ini`/`.cfg`,以及对 `.json`/`.toml`/`.yaml` 的基础嵌套对象展平)。不认识的扩展名会被拒绝。
- **可扩展性**:架构灵活,可按需扩展以满足更复杂的 CLI 需求。

Flag 解析有意采用 pflag 风格:`--k=v`、`-k=v`、短选项分组(`-abc`)、布尔取反(`--no-foo`)、重复出现以及可选值均已支持。启用命令遍历时,Clasp 将子命令发现与可选 flag 值分开处理,因此命名子命令的 token 不会被当作 `NoOptDefVal` flag 的值。

## 安装

### 前置要求

- **C++17**:Clasp 以 C++17 为目标,以获得更广泛的编译器兼容性。

### 从源码构建

```bash
git clone https://github.com/cuihairu/clasp.git
cd clasp
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build
# 或者:cmake --install build --prefix /your/prefix
```

若只需构建/安装库本身(不含示例/CTest),请在配置时加上 `-DCLASP_BUILD_EXAMPLES=OFF`。

## 快速开始

### 基础示例

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

### 运行示例

```bash
./app print --message "Hello, Clasp!"
```

### 输出:

```
Hello, Clasp!
```

## 文档

- `EXAMPLES.md`:所有可运行示例的索引及其演示内容。
- `COMPAT.md`:本项目对“类 Cobra(Cobra-like)”的具体界定。
- `CHANGELOG.md`:重要变更与发布说明。
- 公共头文件位于 `include/clasp/`(`clasp/clasp.hpp` 包含主要 API)。
- `docs/`:VuePress 站点源码(可选)。

## 行为说明

- Flag 优先级为 `命令行 > 环境变量 > 配置文件 > 默认值`。
- `configFile("path")` 会硬编码配置路径;`configFileFlag("config")` 允许通过 `--config` 这类 flag 提供路径。
- 支持的配置格式:`.env`、`.ini`、`.cfg`、`.json`、`.toml`、`.yaml`/`.yml`。不认识的扩展名会被拒绝。
- 补全功能支持生成的 shell 脚本,以及动态的 `__complete` / `__completeNoDesc` 命令。
- 可通过 `markFlagFilename(...)` 和 `markPersistentFlagFilename(...)`,或 `markFlagDirname()` 和 `markPersistentFlagDirname()`,为局部和持久 flag 附加文件/目录补全。
- 在 Visual Studio 等多配置生成器上,请显式指定配置来运行测试,例如 `ctest --test-dir build -C Debug --output-on-failure`。

## 覆盖率

测试覆盖面很广,但覆盖率报告的生成取决于工具链支持:

- GCC/Clang:配置时加上 `-DCLASP_ENABLE_COVERAGE=ON`,并使用你惯用的 `gcov`/`lcov` 流程。
- Visual Studio/MSVC:库和测试可以正常构建运行,但 `CLASP_ENABLE_COVERAGE` 不会为 MSVC 构建插桩。如需 Windows 覆盖率报告,请使用 Visual Studio 自带的代码覆盖率工具。

### VuePress 文档(可选)

```bash
cd docs
npm install
npm run dev
```

## 版本

Clasp 遵循 SemVer。`CMakeLists.txt` 中的版本号与 `include/clasp/clasp.hpp` 中的 `CLASP_VERSION_*` 宏保持一致。

## 在 CMake 中使用

安装之后(或将 `CMAKE_PREFIX_PATH` 指向安装前缀),即可按如下方式使用 Clasp:

```cmake
find_package(clasp CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE clasp::clasp)
```

## 参与贡献

欢迎提交 Issue 和 PR。

## 许可证

Clasp 基于 Apache License 2.0 许可发布。详情见 [LICENSE](LICENSE)。
