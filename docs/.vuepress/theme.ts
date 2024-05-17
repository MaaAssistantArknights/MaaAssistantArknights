import { hopeTheme } from "vuepress-theme-hope";
import DocSearchConfig from './plugins/search';
import { zhcnNavbar, zhtwNavbar, enusNavbar, jajpNavbar, kokrNavbar } from "./navbar/index";
import { zhcnSidebar, zhtwSidebar, enusSidebar, jajpSidebar, kokrSidebar } from "./sidebar/index";

export default hopeTheme({
  hostname: "https://maa.plus",
  iconAssets: "iconify",
  repo: "MaaAssistantArknights/MaaAssistantArknights",
  docsBranch: "dev",
  docsDir: "/docs",

  locales: {
    "/zh-cn/": {
      navbar: zhcnNavbar,
      sidebar: zhcnSidebar,
      displayFooter: false,
      metaLocales: {
        editLink: "在Github上编辑此页面",
      },
    },
    "/zh-tw/": {
      navbar: zhtwNavbar,
      sidebar: zhtwSidebar,
      displayFooter: false,
      metaLocales: {
        editLink: "在Github上編輯此頁面",
      },
    },
    "/en-us/": {
      navbar: enusNavbar,
      sidebar: enusSidebar,
      displayFooter: false,
      metaLocales: {
        editLink: "Edit this page on Github",
      },
    },
    "/ja-jp/": {
      navbar: jajpNavbar,
      sidebar: jajpSidebar,
      displayFooter: false,
      metaLocales: {
        editLink: "このページをGithubで編集する",
      },
    },
    "/ko-kr/": {
      navbar: kokrNavbar,
      sidebar: kokrSidebar,
      displayFooter: false,
      metaLocales: {
        editLink: "Github에서 이 페이지를 편집",
      },
    },
  },

  sidebarIcon: true,

  plugins: {
    activeHeaderLinks: false,

    comment: {
      provider: "Giscus",
      repo: "MaaAssistantArknights/maa-website",
      repoId: "R_kgDOHY7Gyg",
      category: "General",
      categoryId: "DIC_kwDOHY7Gys4CYefe",
      mapping: "title",
      strict: false,
    },

    copyright: {
      license: "AGPL-3.0",
    },

    docsearch: DocSearchConfig,

    mdEnhance: {
      align: true,
      codetabs: true,
      //echarts: true,
      footnote: true,
      gfm: true,
      hint: true,
      imgLazyload: true,
      imgMark: true,
      imgSize: true,
      //mathjax: true,
      mark: true,
      //mermaid: true,
      tabs: true,
      tasklist: true,
      vPre: true,
    },

    prismjs: {
      light: "one-dark",
      dark: "nord",
    },

    sitemap: true,

  },

});
