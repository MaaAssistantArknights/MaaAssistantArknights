import { docsearchPlugin } from "@vuepress/plugin-docsearch";

export default function SearchPlugin() {
  return docsearchPlugin({
    appId: '99JM20SIFG',
    apiKey: '32ad931040facd36d2e99c3cb8e425e1',
    indexName: 'maa',
    locales: {
      '/': {
        placeholder: '搜索',
        translations: {
          button: {
            buttonText: '搜索',
          },
          modal: {
            searchBox: {
              resetButtonTitle: '清除查询条件',
              cancelButtonText: '取消',
            },
            startScreen: {
              recentSearchesTitle: '搜索历史',
              noRecentSearchesText: '没有搜索历史',
              saveRecentSearchButtonTitle: '保存至搜索历史',
              removeRecentSearchButtonTitle: '从搜索历史中移除',
              favoriteSearchesTitle: '收藏',
              removeFavoriteSearchButtonTitle: '从收藏中移除',
            },
            errorScreen: {
              titleText: '无法获取结果',
              helpText: '你可能需要检查你的网络连接',
            },
            footer: {
              selectText: '选择',
              navigateText: '切换',
              closeText: '关闭',
              searchByText: '搜索提供者',
            },
            noResultsScreen: {
              noResultsText: '无法找到相关结果',
              suggestedQueryText: '你可以尝试查询',
              reportMissingResultsText: '你认为该查询应该有结果？',
              reportMissingResultsLinkText: '点击反馈',
            },
          }
        }
      },
      '/zh-tw/': {
        placeholder: '搜尋',
        translations: {
          button: {
            buttonText: '搜尋',
          },
          modal: {
            searchBox: {
              resetButtonTitle: '清除查詢條件',
              cancelButtonText: '取消',
            },
            startScreen: {
              recentSearchesTitle: '搜尋歷史',
              noRecentSearchesText: '沒有搜尋歷史',
              saveRecentSearchButtonTitle: '保存至搜尋歷史',
              removeRecentSearchButtonTitle: '從搜尋歷史中移除',
              favoriteSearchesTitle: '收藏',
              removeFavoriteSearchButtonTitle: '從收藏中移除',
            },
            errorScreen: {
              titleText: '無法獲取結果',
              helpText: '你可能需要檢查你的網路連接',
            },
            footer: {
              selectText: '選擇',
              navigateText: '切換',
              closeText: '關閉',
              searchByText: '搜尋提供者',
            },
            noResultsScreen: {
              noResultsText: '無法找到相關結果',
              suggestedQueryText: '你可以嘗試查詢',
              reportMissingResultsText: '你認為該查詢應該有結果？',
              reportMissingResultsLinkText: '點擊反饋',
            },
          }
        }
      },
      '/en-us/': {
        placeholder: 'Search',
        translations: {
          button: {
            buttonText: 'Search',
          },
          modal: {
            searchBox: {
              resetButtonTitle: 'Clear the query',
              cancelButtonText: 'Cancel',
            },
            startScreen: {
              recentSearchesTitle: 'Recent',
              noRecentSearchesText: 'No recent searches',
              saveRecentSearchButtonTitle: 'Save this search',
              removeRecentSearchButtonTitle: 'Remove this search from history',
              favoriteSearchesTitle: 'Favorite',
              removeFavoriteSearchButtonTitle: 'Remove this search from favorites',
            },
            errorScreen: {
              titleText: 'Unable to fetch results',
              helpText: 'You might want to check your network connection.',
            },
            footer: {
              selectText: 'to select',
              navigateText: 'to navigate',
              closeText: 'to close',
              searchByText: 'Search by',
            },
            noResultsScreen: {
              noResultsText: 'No results for',
              suggestedQueryText: 'Try searching for',
              reportMissingResultsText: 'Believe this query should return results?',
              reportMissingResultsLinkText: 'Let us know.',
            }
          }
        }
      },
      '/ja-jp/': {
        placeholder: '検索する',
      }
    }
  })
}
