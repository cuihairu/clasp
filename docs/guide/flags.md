# Flags (pflag-like Parsing)

## Declaration and Types

Use `withFlag(long, short, var, desc, defaultValue)` to declare flags. Built-in value types follow C++ types:

- `bool/int/int64/uint64/float/double/std::chrono::milliseconds/std::string`
- Extra helpers: `withCountFlag()`, `withBytesFlag()`, `withIPFlag/withCIDRFlag/withIPNetFlag/withIPMaskFlag/withURLFlag()`

## Common Parsing Semantics

- `--k=v`, `-k=v`, `-k v`
- `-abc` (short grouping, configurable)
- `--no-foo` (bool negation, configurable)
- `--` ends flag parsing
- Repeated flags: preserves occurrence order when specified multiple times

## NoOptDefVal (Optional Values)

Use `markFlagNoOptDefaultValue("--mode", "auto")` to allow `--mode` to take a default value when specified without an argument.
When command traversal is enabled, subsequent tokens that are subcommand names will not be consumed as the value for this flag.

## pflag-style Slice/Map Values

Clasp doesn't enforce "typed slice/map flags", but provides parsing helpers (based on repeated values/comma separation):

- `Parser::getStringSlice/getIntSlice/...`
- `Parser::getStringToString/getStringToInt/...`
