# Clasp Compatibility Scope

This document defines what “Cobra-like” means for Clasp, so parity work is measurable and regressions are catchable.

## Compatibility Target (What We Mean By “Cobra Parity”)

Clasp targets **Cobra v1.x user-facing behavior**, not Cobra’s Go API surface.

“Parity” in this repo means: for a given command tree + argv, Clasp matches Cobra’s **observable CLI behavior**:

- Exit code conventions (success vs parse/usage errors).
- `stdout`/`stderr` routing (including `SilenceUsage` / `SilenceErrors` behavior).
- Help/usage/version output sections and defaults (within Clasp’s template system).
- Flag parsing semantics and edge cases (to the extent defined below).
- Completion protocol behavior (`__complete` candidates + directives).

## Audience

- Applications that want Cobra-style command trees, flags, help/usage output, and shell completion.
- C++17 projects that want a small header-first API (minimal build plumbing).

## Compatibility Levels

### Must Support (Compatibility Surface)

These are treated as compatibility commitments; changes should include an example + CTest update.

- **Command tree**: subcommands, aliases, hidden/deprecated commands, groups, suggestions.
- **Hooks**: persistent and local hooks (ordering and inheritance).
- **Args validation**: NoArgs/Exact/Min/Max/Range-style behavior (via validators).
- **Flags**: persistent vs local inheritance; required/hidden/deprecated; group constraints; repeated flags.
- **Parsing ergonomics**: `--k=v`, `-k v`, `-k=v`, `-abc` short grouping, `-ovalue` shorthand value, `--no-foo` bool negation, `--` end-of-flags.
- **Help/usage/version**: `--help/-h`, `help [cmd]`, default values display, stable sorting (with opt-out), examples, version output.
- **Completion**: bash/zsh/fish/powershell script generation, `__complete` callbacks, directives (keep-order, no-file-comp, file-ext, dirs).
- **External sources merge**: precedence `flag > env > config > default` for declared bindings.

## Feature Inventory (Cobra → Clasp)

This section is a practical mapping from commonly used Cobra/pflag concepts to Clasp API and examples.

### Commands & Execution

| Cobra concept | Clasp support | Notes / examples |
|---|---|---|
| `Use/Short/Long` | Supported | `Command(name, short, long)` |
| Subcommands | Supported | `addCommand()` |
| Aliases | Supported | `aliases()` / `addAlias()` |
| Hidden/deprecated commands | Supported | `hidden()` / `deprecated()` |
| Command grouping (`AddGroup`) | Supported | `addGroup()` + `groupId()` |
| Hooks (`PreRun`, `PostRun`, persistent variants) | Supported | `preRun/preRunE/postRun/postRunE` + persistent variants; `examples/hooks_example.cpp` |
| Args validators | Supported | `args(validator)` + helpers `NoArgs/ExactArgs/MinimumNArgs/MaximumNArgs/RangeArgs` |
| Suggestions (unknown cmd/flag) | Supported | `suggestions()` + `suggestionsMinimumDistance()`; `examples/no_suggest_example.cpp` |
| Silence behavior | Supported | `silenceUsage()` / `silenceErrors()`; `examples/silence_errors_example.cpp` |
| Programmatic args execution | Supported | `setArgs(std::vector<std::string>)` + `execute()`; `examples/execute_example.cpp` |
| Context propagation (`ExecuteContext`) | Supported | `setContext()` / `runWithContext()` / `executeWithContext()`; `examples/context_example.cpp` |

### Flags & Parsing (pflag-like Semantics)

| Cobra/pflag concept | Clasp support | Notes / examples |
|---|---|---|
| Local vs persistent flags | Supported | `withFlag()` / `withPersistentFlag()` |
| Required/hidden/deprecated flags | Supported | `markFlagRequired/Hidden/Deprecated` (and persistent variants); `examples/required_example.cpp` |
| Flag groups | Supported | `markFlagsMutuallyExclusive/OneRequired/RequiredTogether`; `examples/flag_groups_example.cpp` |
| `--k=v`, `-k=v`, `-k v` | Supported | Parser supports `=` form and split args |
| `--` end-of-flags | Supported | Tokens after `--` treated as positional |
| Short grouping (`-abc`) | Supported | Enabled by default; configurable via `shortFlagGrouping()`; `examples/parser_knobs_example.cpp` |
| Bool negation (`--no-foo`) | Supported | Enabled by default; configurable via `boolNegation()`; `examples/parser_knobs_example.cpp` |
| `TraverseChildren` | Supported | `traverseChildren()`; `examples/traverse_example.cpp` |
| Allow unknown flags | Supported | `allowUnknownFlags()`; `examples/parser_knobs_example.cpp` |
| Key normalization | Supported | `normalizeFlagKeys()`; `examples/normalize_example.cpp` |
| Repeated flags | Supported | Multiple occurrences preserved; `examples/repeat_example.cpp` |
| `NoOptDefVal` | Supported | `markFlagNoOptDefaultValue()`; `examples/noopt_example.cpp` |
| Built-in types | Supported | `bool/int/int64/uint64/float/double/duration/string`; `examples/types_example.cpp` |
| Extra helpers: `count` | Supported | `withCountFlag()` + `Parser::getCount()`; `examples/count_example.cpp` |
| Extra helpers: bytes | Supported | `withBytesFlag()`; `examples/bytes_example.cpp` |
| Extra helpers: IP/CIDR | Supported | `withIPFlag()` / `withCIDRFlag()`; `examples/net_example.cpp` |
| Extra helpers: IPNet/IPMask | Supported | `withIPNetFlag()` / `withIPMaskFlag()`; `examples/net_extra_example.cpp` |
| Extra helpers: URL | Supported | `withURLFlag()`; `examples/url_example.cpp` |
| Custom `Value` types | Supported | `withValueFlag()` + `clasp::Value`; `examples/custom_value_example.cpp` |
| pflag-like slice/array/map getters | Supported | `Parser` helpers; `examples/pflag_types_example.cpp` |

### Help / Usage / Version Output

| Cobra concept | Clasp support | Notes / examples |
|---|---|---|
| `--help/-h` | Supported | Always available (built-in); `examples/basic_example.cpp` |
| `help [cmd]` command | Supported | Root-only; `enableHelp()` / `disableHelpCommand()`; `examples/help_command_rename_example.cpp` |
| Version output | Supported | `version()` + `--version` + `version` subcommand; `examples/basic_example.cpp` |
| Custom templates | Supported (placeholder-based) | `setHelpTemplate/setUsageTemplate/setVersionTemplate`; `examples/version_template_example.cpp` |
| Full override funcs | Supported | `setHelpFunc()` / `setUsageFunc()` |
| Hide `[flags]` in use line | Supported | `disableFlagsInUseLine()`; `examples/disable_flags_in_use_line_example.cpp` |
| Sorting defaults / opt-out | Supported | `disableSortCommands()` / `disableSortFlags()`; `examples/sorting_example.cpp` |
| Customize parse error text | Supported | `setFlagErrorFunc()`; `examples/flag_error_func_example.cpp` |

### Completion

| Cobra concept | Clasp support | Notes / examples |
|---|---|---|
| Completion scripts (bash/zsh/fish/powershell) | Supported | `printCompletion*`; `examples/completion_example.cpp` |
| Completion command naming / disable | Supported | `enableCompletion(CompletionConfig)`; `examples/completion_naming_example.cpp` |
| `__complete` candidates + directives | Supported | `validArgs/validArgsFunction` + directives; `examples/directive_example.cpp` |
| Flag value completion | Supported | `registerFlagCompletion()`; `examples/dynamic_completion_example.cpp` |
| File/dir helpers | Supported | `markFlagFilename()` / `markFlagDirname()`; `examples/file_completion_example.cpp` |

### Should Support (Best-Effort Fidelity)

These are important for a “Cobra-like feel”, but are not yet treated as a strict contract:

- Byte-for-byte matching of Cobra’s **exact** default English strings and whitespace across versions.
- Full coverage of “weird” pflag edge cases (ordering, error wording, obscure interactions) beyond the test suite.
- Shell-specific completion behavior that is implemented by the shell/runtime (Clasp emits directives; the shell enforces them).

### Non-Goals (Explicitly Out of Scope)

- **Go template parity** for help/usage/version output. Cobra uses Go templates with a rich data model and functions.
  - Clasp templates are **simple placeholder substitution** plus full overrides via `setHelpFunc`/`setUsageFunc`.
- **Full Cobra/pflag API replication** (`*pflag.FlagSet` surface, all Cobra struct fields, and all internal hooks).
- **Full pflag type parity**. Clasp supports a pragmatic subset of built-in types; additional types can be implemented via `clasp::Value`.
- **Complete upstream completion script parity**. Script output is compatible in behavior, but not guaranteed to be identical text.
- **Full Viper compatibility**. Config/env merge exists for convenience, but it is not intended to be a drop-in replacement.

## Current Gaps / Known Differences (Compared To Cobra)

This list is what we currently consider “acceptable differences” under the current scope; turning items from here into “Must Support”
requires adding tests and tightening the contract.

- **Template language**: `setHelpTemplate` / `setUsageTemplate` support only:
  - `{{.UsageLine}}`, `{{.ShortSection}}`, `{{.ExamplesSection}}`, `{{.CommandsSection}}`, `{{.FlagsSection}}`,
    `{{.GlobalFlagsSection}}`, `{{.CommandPath}}`.
  - `setVersionTemplate` supports: `{{.Version}}`, `{{.CommandPath}}`, `{{.Name}}`.
  - No conditionals/loops/pipelines/custom funcs like Cobra’s Go templates.
- **Built-in flags**: Clasp always recognizes `--help/-h` and `--version` as built-ins, even if not explicitly declared.
- **Type surface**: built-in flag value types are limited to `bool/int/int64/uint64/float/double/duration/string` (+ `bytes`, `count`,
  `ip`, `ipmask`, `cidr`, `ipnet`, and `url` helpers implemented via annotations). Broader pflag type parity is out of scope unless added
  intentionally.
- **Completion fidelity**: file/dir filtering is directive-driven; exact behavior depends on the shell completion runtime.
- **Error text**: error messages aim to be Cobra-like, but are not guaranteed to match Cobra’s exact phrasing unless locked by tests.

## How We Measure Parity Here

- The CTest suite in `CMakeLists.txt` exercises example executables under `examples/` and asserts behavior/output.
- If you find a Cobra behavior gap, the preferred workflow is:
  1) Add a reproducer example or extend an existing one in `examples/`.
  2) Add/adjust a CTest assertion for the desired behavior.
  3) Implement the behavior change.

## Versioning Notes

The project version is defined in `include/clasp/clasp.hpp` and `CMakeLists.txt`. Compatibility-affecting behavior changes should be
accompanied by an example + CTest update where feasible.
