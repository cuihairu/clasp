import { defineConfig } from 'vitepress'

export default defineConfig({
  // Base path for GitHub Pages
  base: '/clasp/',

  // Shared configuration
  title: 'Clasp',
  description: 'C++17 CLI library with Cobra-like command tree and pflag-like parsing',

  // locales configuration
  locales: {
    root: {
      label: 'English',
      lang: 'en',
      themeConfig: {
        nav: [
          { text: 'Guide', link: '/guide/' },
          { text: 'Reference', link: '/reference/' }
        ],
        sidebar: {
          '/guide/': [
            {
              text: 'Guide',
              items: [
                { text: 'Getting Started', link: '/guide/' },
                { text: 'Commands', link: '/guide/commands' },
                { text: 'Flags', link: '/guide/flags' },
                { text: 'Completion', link: '/guide/completion' },
                { text: 'Config', link: '/guide/config' },
                { text: 'Templates', link: '/guide/templates' },
                { text: 'Extending', link: '/guide/extending' }
              ]
            }
          ],
          '/reference/': [
            {
              text: 'Reference',
              items: [
                { text: 'Compatibility', link: '/reference/compat' },
                { text: 'API', link: '/reference/api' },
                { text: 'Examples', link: '/reference/examples' }
              ]
            }
          ]
        },
        editLink: {
          pattern: 'https://github.com/cuihairu/clasp/edit/main/docs/:path',
          text: 'Edit this page on GitHub'
        },
        lastUpdated: {
          text: 'Last Updated',
          formatOptions: {
            dateStyle: 'full',
            timeStyle: 'short'
          }
        },
        socialLinks: [
          { icon: 'github', link: 'https://github.com/cuihairu/clasp' }
        ]
      }
    },
    zh: {
      label: '简体中文',
      lang: 'zh-CN',
      themeConfig: {
        nav: [
          { text: '指南', link: '/zh/guide/' },
          { text: '参考', link: '/zh/reference/' }
        ],
        sidebar: {
          '/zh/guide/': [
            {
              text: '指南',
              items: [
                { text: '快速开始', link: '/zh/guide/' },
                { text: '命令', link: '/zh/guide/commands' },
                { text: '标志位', link: '/zh/guide/flags' },
                { text: '补全', link: '/zh/guide/completion' },
                { text: '配置', link: '/zh/guide/config' },
                { text: '模板', link: '/zh/guide/templates' },
                { text: '扩展', link: '/zh/guide/extending' }
              ]
            }
          ],
          '/zh/reference/': [
            {
              text: '参考',
              items: [
                { text: '兼容性', link: '/zh/reference/compat' },
                { text: 'API', link: '/zh/reference/api' },
                { text: '示例', link: '/zh/reference/examples' }
              ]
            }
          ]
        },
        editLink: {
          pattern: 'https://github.com/cuihairu/clasp/edit/main/docs/:path',
          text: '在 GitHub 上编辑此页'
        },
        lastUpdated: {
          text: '最后更新',
          formatOptions: {
            dateStyle: 'full',
            timeStyle: 'short'
          }
        },
        socialLinks: [
          { icon: 'github', link: 'https://github.com/cuihairu/clasp' }
        ]
      }
    }
  }
})
