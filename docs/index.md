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

<script setup lang="ts">
// 首次访问按浏览器语言选择文档语言（vitepress 无此能力，GitHub Pages 不支持 _redirects）
// 优先级：localStorage 手动选择 > navigator.language > 保持当前 locale
import { onBeforeMount } from 'vue'
const KEY = 'clasp-lang-auto-picked'
const BASE = '/clasp/'
const ROOT_PATH = BASE
const ZH_PATH = BASE + 'zh/'
onBeforeMount(() => {{
  if (typeof window === 'undefined') return
  const p = window.location.pathname
  if (p !== ROOT_PATH && p !== ROOT_PATH.slice(0, -1)) return
  const picked = localStorage.getItem(KEY)
  if (picked) {{ if (picked !== p) window.location.replace(picked); return }}
  const langs = (navigator.languages?.length ? navigator.languages : [navigator.language]).map(l => l.toLowerCase())
  if (langs.some(l => l.startsWith('zh'))) window.location.replace(ZH_PATH)
}})
if (typeof window !== 'undefined') {{
  window.addEventListener('click', (e) => {{
    const a = (e.target as HTMLElement)?.closest?.('a') as HTMLAnchorElement | null
    const href = a?.getAttribute('href') || ''
    if (href.startsWith(ZH_PATH) || href.startsWith('zh/')) localStorage.setItem(KEY, ZH_PATH)
    else if (href === ROOT_PATH || href === ROOT_PATH.slice(0, -1)) localStorage.setItem(KEY, ROOT_PATH)
  }}, true)
}}
</script>


## What is Clasp?

Clasp is a C++17 CLI library. The goal is not to replicate Cobra's Go API, but to align with Cobra v1.x's **observable CLI behaviors** (help/usage, parsing semantics, completion protocol, etc.).

## Next Steps

- Start with `guide/`: installation, minimal examples, CMake integration.
- Check `reference/compat` for compatibility scope and differences.
