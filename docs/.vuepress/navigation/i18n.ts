export interface Locale {
  linkName: string;
  dirName: string;
  // displayName: string;
  htmlLang: string;
  siteTitle: string;
  siteDescription: string;
}

export const locales: Locale[] = [
  {
    linkName: 'zh-cn',
    dirName: 'zh-cn',
    // displayName: '简体中文',
    htmlLang: 'zh-CN',
    siteTitle: 'MAA 文档站',
    siteDescription: '文档',
  },
  {
    linkName: 'zh-tw',
    dirName: 'zh-tw',
    // displayName: '繁體中文',
    htmlLang: 'zh-TW',
    siteTitle: 'MAA 文件站',
    siteDescription: '文件',
  },
  {
    linkName: 'en-us',
    dirName: 'en-us',
    // displayName: 'English',
    htmlLang: 'en-US',
    siteTitle: 'MAA Documentation Site',
    siteDescription: 'Documentation',
  },
  {
    linkName: 'ja-jp',
    dirName: 'ja-jp',
    // displayName: '日本語',
    htmlLang: 'ja-JP',
    siteTitle: 'MAA ドキュメントサイト',
    siteDescription: 'ドキュメント',
  },
  {
    linkName: 'ko-kr',
    dirName: 'ko-kr',
    // displayName: '한국어',
    htmlLang: 'ko-KR',
    siteTitle: 'MAA 문서 사이트',
    siteDescription: '문서',
  },
];
