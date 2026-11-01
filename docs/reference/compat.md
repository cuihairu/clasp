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
- Canonical Cobra-like strings: common failure messages (unknown command/flag + suggestions, needs-argument, invalid argument, required-flag-not-set) are locked byte-for-byte by CTest (`examples/cobra_strings_example.cpp`)
- pflag edge interactions: repeated bools, empty `--flag=`, negative-number values, intermixed positionals, `NoOptDefVal` both forms, `--` end-of-flags, inline short values, repeated values last-wins — pinned by behavior tests (`examples/pflag_edge_example.cpp`)
- Completion directives per shell: the directive-handling logic of every generated script (bash/zsh/fish/powershell) is asserted by CTest; shell-runtime enforcement and byte-identical scripts remain out of scope

The three bullets above were promoted from the former "Should Support (Best-Effort Fidelity)" list; each now carries its own example + CTest coverage (see `COMPAT.md`).

## Best-Effort Fidelity (Promoted to Must Support)

These were previously best-effort; they are now compatibility commitments with per-item example + CTest locks (see Must Support above):
- Byte-for-byte consistency with Cobra's default English text/whitespace → `cobra_strings_example.cpp`
- More unusual pflag edge cases (sorting, error text details, rare interactions) → `pflag_edge_example.cpp`
- Completion script details in shell/runtime (Clasp outputs directives; per-shell directive handling is asserted) → completion CTest matrix

## Non-Goals (Explicitly Not Pursued)

- Go template language equivalence (Clasp only does placeholder replacement + complete override functions)
- Copying the entire Cobra/pflag Go API surface (e.g., `*pflag.FlagSet`)
- Full pflag type surface (can extend via `clasp::Value`)
- Full Viper drop-in replacement

## Known Differences (Currently Acceptable)

- Template language capability limited (no conditions/loops/pipes/funcs)
- `--help/-h` and `--version` as built-in flags (even when not explicitly declared)
- Built-in type surface is a subset (`bool/int/int64/uint32/uint64/float/double/duration/string` + narrow-width/bytes/count/ip/cidr/ipnet/ipmask/url/ipSlice/bytesBase64 helpers)
- Completion details depend on shell/runtime
- Error text not guaranteed byte-for-byte consistent (unless locked by CTest)

## Recommended Process to Make Best-Effort Into Contract

1) Add reproducer/examples in `examples/`
2) Add CTest assertions in `CMakeLists.txt` (lock output/exit code/key semantics)
3) Implement behavior and ensure cross-platform consistency (macOS/Linux/Windows)
