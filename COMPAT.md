# Clasp Compatibility Scope

This document defines what “Cobra-like” means for Clasp, so parity work is measurable.

## Target Audience

- Applications that want Cobra-style command trees, flags, help/usage output, and shell completion.
- C++17 projects that prefer a small header-first API (with a minimal build).

## Compatibility Goals

Clasp aims to match Cobra’s user-facing CLI behavior for:

- Command tree: subcommands, aliases, hidden/deprecated commands, suggestions.
- Flags: persistent vs local, required/hidden/deprecated flags, grouping constraints, `--` end-of-flags.
- Parsing ergonomics: short-flag grouping (`-abc`), bool negation (`--no-foo`), child flags before subcommands (`TraverseChildren`).
- Help/usage/version: `--help/-h`, `help [cmd]`, default values, stable sorting, examples, and version templates.
- Completion: generation for bash/zsh/fish/powershell, `__complete`-style callbacks, and directives.
- External sources: env and config file merge (flag > env > config > default).

These behaviors are validated via the CTest suite wired in `CMakeLists.txt`.

## Non-Goals (For Now)

Clasp does not attempt to fully replicate Cobra/pflag’s internal API surface and every edge-case:

- Full pflag type parity (all slice/map types, full Value/edge-case surface, advanced normalization hooks).
- Full completion semantics (file/dir filtering behavior enforced by the shell; Clasp only emits directives).
- Full config formats (YAML/TOML/JSON are supported, but only a minimal subset is built-in).

## Versioning

The project version is defined in `clasp/clasp.hpp` and `CMakeLists.txt`. Behavior changes should be accompanied by a new example/CTest where feasible.
