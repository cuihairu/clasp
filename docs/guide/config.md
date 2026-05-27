# Config (env/config Merging)

Clasp provides viper-like "external source" merging: `flag > env > config > default`.

## Binding env

Use `bindEnv("--flag", "ENV_NAME")`.

## Config Files

- Specify config file: `configFile("path")`
- Provide a config file flag like `--config`: `configFileFlag("config")`

Supports `.env` style, JSON/TOML/YAML (see `examples/config_*`).
