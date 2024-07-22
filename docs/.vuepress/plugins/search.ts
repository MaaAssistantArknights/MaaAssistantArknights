export default {
    appId: "99JM20SIFG",
    apiKey: "7596a5a8c95cd64d4cf3050c9a4f878e",
    indexName: "maa",
    // 这里应该把所有本地化配置都放入 locales 中
    locales: {
        "/zh-cn/": {
            placeholder: "搜索",
            translations: {
                button: {
                    buttonText: "搜索",
                },
                modal: {
                    searchBox: {
                        resetButtonTitle: "清除查询条件",
                        cancelButtonText: "取消",
                    },
                    startScreen: {
                        recentSearchesTitle: "搜索历史",
                        noRecentSearchesText: "没有搜索历史",
                        saveRecentSearchButtonTitle: "保存至搜索历史",
                        removeRecentSearchButtonTitle: "从搜索历史中移除",
                        favoriteSearchesTitle: "收藏",
                        removeFavoriteSearchButtonTitle: "从收藏中移除",
                    },
                    errorScreen: {
                        titleText: "无法获取结果",
                        helpText: "你可能需要检查你的网络连接",
                    },
                    footer: {
                        selectText: "选择",
                        navigateText: "切换",
                        closeText: "关闭",
                        searchByText: "搜索提供者",
                    },
                    noResultsScreen: {
                        noResultsText: "无法找到相关结果",
                        suggestedQueryText: "你可以尝试查询",
                        reportMissingResultsText: "你认为该查询应该有结果？",
                        reportMissingResultsLinkText: "点击反馈",
                    },
                },
            },
        },
        "/en-us/": {
            placeholder: "Search",
            translations: {
                button: {
                    buttonText: "Search",
                },
                modal: {
                    searchBox: {
                        resetButtonTitle: "Clear the query",
                        cancelButtonText: "Cancel",
                    },
                    startScreen: {
                        recentSearchesTitle: "Recent",
                        noRecentSearchesText: "No recent searches",
                        saveRecentSearchButtonTitle: "Save this search",
                        removeRecentSearchButtonTitle:
                            "Remove this search from history",
                        favoriteSearchesTitle: "Favorite",
                        removeFavoriteSearchButtonTitle:
                            "Remove this search from favorites",
                    },
                    errorScreen: {
                        titleText: "Unable to fetch results",
                        helpText:
                            "You might want to check your network connection.",
                    },
                    footer: {
                        selectText: "to select",
                        navigateText: "to navigate",
                        closeText: "to close",
                        searchByText: "Search by",
                    },
                    noResultsScreen: {
                        noResultsText: "No results for",
                        suggestedQueryText: "Try searching for",
                        reportMissingResultsText:
                            "Believe this query should return results?",
                        reportMissingResultsLinkText: "Let us know.",
                    },
                },
            },
        },
    },
};
