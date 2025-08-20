import { viteBundler } from '@vuepress/bundler-vite'
import { defineUserConfig } from 'vuepress'
import { googleAnalyticsPlugin } from "@vuepress/plugin-google-analytics";
import { plumeTheme } from 'vuepress-theme-plume'

import DocSearchConfig from './plugins/search';

const isProd = process.env.NODE_ENV === 'production';

export default defineUserConfig({
  base: "/",
  lang: "zh-CN",
  title: "MaaAssistantArknights",
  description: "MAA",
  port: 3001,

  locales: {
    "/zh-cn/": {
      lang: "zh-CN",
      description: "文档",
    },
    "/zh-tw/": {
      lang: "zh-TW",
      description: "文件",
    },
    "/en-us/": {
      lang: "en-US",
      description: "Documentation",
    },
    "/ja-jp/": {
      lang: "ja-JP",
      description: "ドキュメンテーション",
    },
    "/ko-kr/": {
      lang: "ko-KR",
      description: "선적 서류 비치",
    },
  },

  head: [
    ["link", { rel: "preconnect", href: "https://fonts.googleapis.com" }],
    [
      "link",
      { rel: "preconnect", href: "https://fonts.gstatic.com", crossorigin: "" },
    ],
    [
      "link",
      {
        href: "https://fonts.googleapis.com/css2?family=Noto+Sans+SC:wght@100..900&display=swap",
        rel: "stylesheet",
      },
    ],
    [
      "link",
      {
        href: "https://fonts.googleapis.com/css2?family=Noto+Serif+SC:wght@200..900&display=swap",
        rel: "stylesheet",
      },
    ],
    [
      "link",
      {
        href: "https://fonts.googleapis.com/css2?family=JetBrains+Mono:ital,wght@0,100..800;1,100..800&display=swap",
        rel: "stylesheet",
      },
    ],
  ],

  bundler: viteBundler({
    viteOptions: {},
    vuePluginOptions: {},
  }),

  shouldPrefetch: false,

  theme: plumeTheme({
    hostname: "https://docs.maa.plus",

    docsRepo: "MaaAssistantArknights/MaaAssistantArknights",
    docsDir: "/docs",
    docsBranch: "dev",

    editLink: true,
    lastUpdated: false,
    contributors: false,
    changelog: false,

    blog: false,

    cache: "filesystem",

    search: DocSearchConfig,

    codeHighlighter: {
      themes: { light: 'one-light', dark: 'one-dark-pro' },
    },

    // markdown: todo

    watermark: false,

    comment: {
      provider: "Giscus",
      repo: "MaaAssistantArknights/maa-website",
      repoId: "R_kgDOHY7Gyg",
      category: "Comments",
      categoryId: "DIC_kwDOHY7Gys4CgoVH",
      mapping: "pathname",
      strict: true,
      lazyLoading: true,
    },

    //replaceAssets: isProd ? "https://cdn.maa.plus" : false,

  }),

  plugins: [
    googleAnalyticsPlugin({
      id: "G-FJQDKG394Z",
    }),
  ],
});
