export interface Locale {
  name: string
  // displayName: string
  htmlLang: string
  siteTitle: string
  siteDescription: string
}

export const locales: Locale[] = [
  {
    name: 'zh-cn',
    // displayName: '简体中文',
    htmlLang: 'zh-CN',
    siteTitle: 'MAA 文档站',
    siteDescription: '文档',
  },
  {
    name: 'zh-tw',
    // displayName: '繁體中文',
    htmlLang: 'zh-TW',
    siteTitle: 'MAA 文件站',
    siteDescription: '文件',
  },
  {
    name: 'en-us',
    // displayName: 'English',
    htmlLang: 'en-US',
    siteTitle: 'MAA Documentation Site',
    siteDescription: 'Documentation',
  },
  {
    name: 'ja-jp',
    // displayName: '日本語',
    htmlLang: 'ja-JP',
    siteTitle: 'MAA ドキュメントサイト',
    siteDescription: 'ドキュメント',
  },
  {
    name: 'ko-kr',
    // displayName: '한국어',
    htmlLang: 'ko-KR',
    siteTitle: 'MAA 문서 사이트',
    siteDescription: '문서',
  },
]
