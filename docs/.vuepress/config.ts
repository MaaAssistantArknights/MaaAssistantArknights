import { defineUserConfig } from "vuepress";
import { getDirname, path } from "vuepress/utils";
import { viteBundler } from "@vuepress/bundler-vite";
import { googleAnalyticsPlugin } from "@vuepress/plugin-google-analytics";
import Theme from "./theme";

const __dirname = getDirname(import.meta.url);

export default defineUserConfig({
  base: "/docs/",
  lang: "zh-CN",
  title: "MaaAssistantArknights",
  description: "MAA",
  port: 3001,

  locales: {
    "/zh-cn/": {
      lang: "zh-cn",
      description: "文档",
    },
    "/zh-tw/": {
      lang: "zh-tw",
      description: "文件",
    },
    "/en-us/": {
      lang: "en-us",
      description: "Documentation",
    },
    "/ja-jp/": {
      lang: "ja-jp",
      description: "ドキュメンテーション",
    },
    "/ko-kr/": {
      lang: "ko-kr",
      description: "선적 서류 비치",
    },
  },

  markdown: {
    headers: {
      level: [2, 3, 4, 5],
    },
  },

  theme: Theme,

  alias: {
    "@theme-hope/modules/navbar/components/LanguageDropdown": path.resolve(
      __dirname,
      "./components/LanguageDropdown.ts",
    ),
  },

  plugins: [
    googleAnalyticsPlugin({
      id: "G-FJQDKG394Z",
    }),
  ],

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
});
