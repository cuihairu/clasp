# Config（env/config 合并）

Clasp 提供 viper-like 的“外部源”合并：`flag > env > config > default`。

## 绑定 env

使用 `bindEnv("--flag", "ENV_NAME")`。

## 配置文件

- 指定配置文件：`configFile("path")`
- 提供 `--config` 之类的配置文件 flag：`configFileFlag("config")`
- `configFileFlag("config")` 按规范化后的 flag 名匹配，因此 `config` 和 `--config` 都可以。

支持的格式：

- `.env`：`KEY=value`
- `.ini` / `.cfg`：section key 会被拍平成 flag 名
- `.json`
- `.toml`
- `.yaml` / `.yml`

说明：

- 未知扩展名会直接报错。
- 环境变量会覆盖配置文件中的值。
- 显式 CLI flag 会覆盖两者。
- 这些解析器刻意只实现适合 CLI 合并的实用子集；支持的结构可参考 `examples/config_*`。
