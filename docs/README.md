---
home: true
title: Clasp
heroText: Clasp
tagline: C++17 的 Cobra-like 命令行框架（兼容行为优先）
actions:
  - text: 快速开始
    link: /guide/getting-started.html
    type: primary
  - text: GitHub
    link: https://github.com/cuihairu/clasp
    type: secondary
features:
  - title: Cobra-like Command Tree
    details: 子命令、aliases、hooks、args validators、TraverseChildren 等。
  - title: pflag-like Parsing
    details: --k=v / -k=v / -abc、--no-foo、NoOptDefVal、重复 flags 语义。
  - title: Completion + Config
    details: bash/zsh/fish/powershell completion + env/config merge（flag > env > config > default）。
footer: Apache-2.0 Licensed
---

## 这是啥

Clasp 是一个 C++17 CLI 库：目标不是复制 Cobra 的 Go API，而是对齐 Cobra v1.x 的**可观测 CLI 行为**（help/usage、解析语义、completion 协议等）。

## 下一步

- 从 `guide/getting-started` 开始：安装、最小示例、CMake 集成。
- 需要对齐范围/差异：看 `reference/compat`。
