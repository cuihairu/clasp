---
layout: home
title: Clasp
hero:
  name: Clasp
  tagline: C++17 CLI library with Cobra-like command tree and pflag-like parsing
  actions:
    - text: Get Started
      link: /guide/
      theme: primary
    - text: GitHub
      link: https://github.com/cuihairu/clasp
      theme: secondary
features:
  - icon: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" width="24" height="24"><circle cx="6" cy="5" r="2.2"/><circle cx="6" cy="19" r="2.2"/><circle cx="18" cy="12" r="2.2"/><path d="M6 7.2v9.6"/><path d="M8.2 5h4.3a3 3 0 0 1 3 3v1.8"/><path d="M8.2 19h4.3a3 3 0 0 0 3-3v-1.8"/></svg>'
    title: Cobra-like Command Tree
    details: Subcommands, aliases, hooks, args validators, TraverseChildren, and more.
  - icon: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" width="24" height="24"><path d="m8 6-4.5 6L8 18"/><path d="m16 6 4.5 6L16 18"/><path d="m13.2 5-2.4 14"/></svg>'
    title: pflag-like Parsing
    details: --k=v / -k=v / -abc short grouping, --no-foo bool negation, NoOptDefVal, repeated flags.
  - icon: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" width="24" height="24"><rect x="3" y="4.5" width="18" height="15" rx="2"/><path d="m7.5 10 2.6 2.6L7.5 15.2"/><path d="M13 15.2h4"/></svg>'
    title: Completion + Config
    details: bash/zsh/fish/powershell completion + env/config merge (flag > env > config > default).
footer: Apache-2.0 Licensed
---

## What is Clasp?

Clasp is a C++17 CLI library. The goal is not to replicate Cobra's Go API, but to align with Cobra v1.x's **observable CLI behaviors** (help/usage, parsing semantics, completion protocol, etc.).

## Next Steps

- Start with `guide/`: installation, minimal examples, CMake integration.
- Check `reference/compat` for compatibility scope and differences.
