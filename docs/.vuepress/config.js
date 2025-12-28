module.exports = {
  lang: 'zh-CN',
  title: 'Clasp',
  description: 'C++17 的 Cobra-like 命令行框架（兼容行为优先）',

  themeConfig: {
    repo: 'cuihairu/clasp',
    docsDir: 'docs',
    editLinks: true,
    lastUpdated: 'Last Updated',

    nav: [
      { text: '指南', link: '/guide/' },
      { text: '参考', link: '/reference/' },
      { text: 'GitHub', link: 'https://github.com/cuihairu/clasp' },
    ],

    sidebar: {
      '/guide/': [
        '',
        'getting-started',
        'commands',
        'flags',
        'completion',
        'config',
        'templates',
        'extending',
      ],
      '/reference/': ['compat', 'api', 'examples'],
    },
  },
}

