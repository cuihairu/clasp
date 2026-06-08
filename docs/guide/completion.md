# Completion

## Generating Scripts

Clasp supports bash/zsh/fish/powershell completion script generation (see `examples/completion_example.cpp`).

Use `enableCompletion()` on the root command to install:

- a user-facing `completion <shell>` command
- internal `__complete` and `__completeNoDesc` commands used by generated scripts

You can rename or disable those commands with `enableCompletion(Command::CompletionConfig{...})`.

## Dynamic Completion (__complete)

- Positional: `validArgs()` / `validArgsFunction()`
- Flag value: `registerFlagCompletion("--flag", func)`

Dynamic completion respects command traversal and subcommand context, so completions are resolved against the command that would actually execute.

## Directives

Supports Cobra-like directives (e.g., no-file-comp, keep-order, file extension filtering, directory filtering, etc.).

Common helpers:

- `markFlagFilename("--config", {"yaml", "yml"})`
- `markFlagDirname("--out-dir")`
- `markPersistentFlagFilename("--config", {"yaml", "yml"})`
- `markPersistentFlagDirname("--out-dir")`

These helpers emit directive metadata consumed by bash/zsh/fish/powershell completion scripts.
