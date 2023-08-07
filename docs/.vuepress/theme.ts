import { hopeTheme } from "vuepress-theme-hope";
import { zhcnNavbar } from "./navbar/index";
import { zhcnSidebar } from "./sidebar/index";

export default hopeTheme({
  iconAssets: "iconfont",
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
  },

});
