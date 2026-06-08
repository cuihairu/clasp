# Config (env/config Merging)

Clasp provides viper-like "external source" merging: `flag > env > config > default`.

## Binding env

Use `bindEnv("--flag", "ENV_NAME")`.

## Config Files

- Specify config file: `configFile("path")`
- Provide a config file flag like `--config`: `configFileFlag("config")`
- `configFileFlag("config")` matches the normalized flag name, so `config` and `--config` both work.

Supported formats:

- `.env`: `KEY=value`
- `.ini` / `.cfg`: section keys are flattened into flag names
- `.json`
- `.toml`
- `.yaml` / `.yml`

Notes:

- Unknown extensions are rejected.
- Environment values override config file values.
- Explicit CLI flags override both.
- The parsers intentionally implement a practical subset aimed at CLI config merging; see `examples/config_*` for supported shapes.
