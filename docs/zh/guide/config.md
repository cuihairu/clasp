# Config（env/config 合并）

Clasp 提供 viper-like 的“外部源”合并：`flag > env > config > default`。

## 绑定 env

使用 `bindEnv("--flag", "ENV_NAME")`。

## 配置文件

- 指定配置文件：`configFile("path")`
- 提供 `--config` 之类的配置文件 flag：`configFileFlag("config")`

支持 `.env` 风格、JSON/TOML/YAML（见 `examples/config_*`）。
