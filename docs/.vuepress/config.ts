const { defaultTheme } = require('@vuepress/theme-default')
const { searchPlugin } = require('@vuepress/plugin-search')

module.exports = {
  lang: 'zh-CN',
  title: 'MaaAssistantArknights',
  description: 'MAA',
  theme: defaultTheme({
    repo: 'MaaAssistantArknights/MaaAssistantArknights',
    sidebarDepth: 5,
    docsBranch: 'lr',
    docsDir: 'docs',
    // editLinkPattern: ':repo/edit/:branch/:path'
    navbar: [{ text: '必读', link: '/guide.html' }],
  }),
  markdown: {
    extractHeaders: {
      level: [2, 3, 4, 5],
    },
  },
  plugins: [
    searchPlugin({ })
  ],
}
