import { hopeTheme } from "vuepress-theme-hope";
import { zhcnNavbar } from "./navbar/index";
import { zhcnSidebar } from "./sidebar/index";

export default hopeTheme({
  iconAssets: "iconify",
  repo: "MaaAssistantArknights/MaaAssistantArknights",
  docsBranch: "dev",
  docsDir: "/docs",

  locales: {
    "/": {
      navbar: zhcnNavbar,
      sidebar: zhcnSidebar,
      displayFooter: false,
      metaLocales: {
        editLink: "在Github上编辑此页面",
      },
    },
    "/zh-tw/": {
      //selectLanguageName: "繁體中文",
    },
    "/en-us/": {
      //selectLanguageName: "English",
    },
    "/ja-jp/": {
      //selectLanguageName: "日本語",
    },
    "/ko-kr/": {
      //selectLanguageName: "한국어",
    },
  },

  sidebarIcon: true,

  plugins: {
    activeHeaderLinks: false,

    mdEnhance: {
      align: true,
      card: true,
      chart: true,
      codetabs: true,
      container: true,
      echarts: true,
      figure: true,
      footnote: true,
      gfm: true,
      imgLazyload: true,
      imgMark: true,
      imgSize: true,
      mathjax: true,
      mark: true,
      mermaid: true,
      tabs: true,
      tasklist: true,
      vPre: true,
    },

    prismjs: {
      light: "one-dark",
      dark: "nord",
    },



  },

});
