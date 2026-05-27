# Compatibility Scope (Cobra v1.x)

This project defines "Cobra parity" as: given the same command tree + argv, Clasp should match Cobra v1.x's **observable CLI behaviors** (not copying the Go API surface).

## Must Support (Compatibility Contract)

These are "compatibility commitments": behavioral changes should include corresponding examples + CTest.

- Command tree: subcommands, aliases, hidden/deprecated, groups, suggestions
- Hooks: persistent + local (inheritance and ordering)
- Args validators: NoArgs/Exact/Min/Max/Range
- Flags: persistent vs local, required/hidden/deprecated, group constraints, repeated flags, NoOptDefVal
- Parsing semantics: `--k=v`, `-k v`, `-k=v`, `-abc`, `-ovalue`, `--no-foo`, `NoOptDefVal`, `--`
- help/usage/version: `--help/-h`, `help [cmd]`, default values display, sorting toggles, examples
- Completion: bash/zsh/fish/powershell + `__complete` candidates + directives
- External source merging: `flag > env > config > default`

## Should Support (Best-Effort)

These are important for "Cobra-like feel" but not strict contracts (unless locked by CTest):

- Byte-for-byte consistency with Cobra's default English text/whitespace
- More unusual pflag edge cases (sorting, error text details, rare interactions)
- Completion script details in shell/runtime (Clasp only outputs directives)

## Non-Goals (Explicitly Not Pursued)

- Go template language equivalence (Clasp only does placeholder replacement + complete override functions)
- Copying the entire Cobra/pflag Go API surface (e.g., `*pflag.FlagSet`)
- Full pflag type surface (can extend via `clasp::Value`)
- Full Viper drop-in replacement

## Known Differences (Currently Acceptable)

- Template language capability limited (no conditions/loops/pipes/funcs)
- `--help/-h` and `--version` as built-in flags (even when not explicitly declared)
- Built-in type surface is a subset (`bool/int/int64/uint64/float/double/duration/string` + helpers)
- Completion details depend on shell/runtime
- Error text not guaranteed byte-for-byte consistent (unless locked by CTest)

## Recommended Process to Make Best-Effort Into Contract

1) Add reproducer/examples in `examples/`
2) Add CTest assertions in `CMakeLists.txt` (lock output/exit code/key semantics)
3) Implement behavior and ensure cross-platform consistency (macOS/Linux/Windows)
