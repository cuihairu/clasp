---
home: true
title: Clasp
heroText: Clasp
tagline: C++17 CLI library with Cobra-like command tree and pflag-like parsing
actions:
  - text: Get Started
    link: /guide/
    type: primary
  - text: GitHub
    link: https://github.com/cuihairu/clasp
    type: secondary
features:
  - title: Cobra-like Command Tree
    details: Subcommands, aliases, hooks, args validators, TraverseChildren, and more.
  - title: pflag-like Parsing
    details: --k=v / -k=v / -abc short grouping, --no-foo bool negation, NoOptDefVal, repeated flags.
  - title: Completion + Config
    details: bash/zsh/fish/powershell completion + env/config merge (flag > env > config > default).
footer: Apache-2.0 Licensed
---

## What is Clasp?

Clasp is a C++17 CLI library. The goal is not to replicate Cobra's Go API, but to align with Cobra v1.x's **observable CLI behaviors** (help/usage, parsing semantics, completion protocol, etc.).

## Next Steps

- Start with `guide/`: installation, minimal examples, CMake integration.
- Check `reference/compat` for compatibility scope and differences.
