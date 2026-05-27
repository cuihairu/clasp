# Completion

## Generating Scripts

Clasp supports bash/zsh/fish/powershell completion script generation (see `examples/completion_example.cpp`).

## Dynamic Completion (__complete)

- Positional: `validArgs()` / `validArgsFunction()`
- Flag value: `registerFlagCompletion("--flag", func)`

## Directives

Supports Cobra-like directives (e.g., no-file-comp, keep-order, file extension filtering, directory filtering, etc.).
