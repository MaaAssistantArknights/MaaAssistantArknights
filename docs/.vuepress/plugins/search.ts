import { searchPlugin } from "@vuepress/plugin-search";

export default function SearchPlugin() {
  return searchPlugin({
    locales: {
      '/': {
        placeholder: '搜索',
      },
      '/zh-tw/': {
        placeholder: '搜索',
      },
      '/en-us/': {
        placeholder: 'Search',
      },
      '/ja-jp/': {
        placeholder: '検索する'
      }
    },
    hotKeys: ['k']
  })
}
