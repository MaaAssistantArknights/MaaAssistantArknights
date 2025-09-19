import { defineThemeConfig } from 'vuepress-theme-plume';
import { zhcnNavbar, zhtwNavbar, enusNavbar, jajpNavbar, kokrNavbar } from './navbar';
import { zhcnNotes, zhtwNotes, enusNotes, jajpNotes, kokrNotes } from './notes';

export default defineThemeConfig({
  logo: '/images/maa-logo_512x512.png',

  appearance: true,

  social: [
    { icon: 'qq', link: 'https://api.maa.plus/MaaAssistantArknights/api/qqgroup' },
    { icon: 'discord', link: 'https://discord.gg/23DfZ9uA4V' },
    //{ icon: 'telegram', link: 'https://t.me/+Mgc2Zngr-hs3ZjU1' },
    { icon: 'bilibili', link: 'https://space.bilibili.com/3493274731940507/' },
    { icon: 'github', link: 'https://github.com/MaaAssistantArknights/MaaAssistantArknights/' },
  ],
  navbarSocialInclude: ['qq', 'discord', 'bilibili', 'github'],

  aside: true,
  // outline: [2, 3, 4, 5],
  copyright: false,
  prevPage: false,
  nextPage: false,
  createTime: false,

  locales: {
    '/zh-cn/': {
      navbar: zhcnNavbar,
      notes: zhcnNotes,
      footer: false,
    },
    '/zh-tw/': {
      navbar: zhtwNavbar,
      notes: zhtwNotes,
      footer: false,
    },
    '/en-us/': {
      navbar: enusNavbar,
      notes: enusNotes,
      footer: false,
    },
    '/ja-jp/': {
      navbar: jajpNavbar,
      notes: jajpNotes,
      footer: false,
    },
    '/ko-kr/': {
      navbar: kokrNavbar,
      notes: kokrNotes,
      footer: false,
    },
  },

  autoFrontmatter: false,

  //markdown: {
  //  align: true,
  //  codeTabs: true,
  //  //echarts: true,
  //  footnote: true,
  //  gfm: true,
  //  hint: true,
  //  imgLazyload: true,
  //  imgMark: true,
  //  imgSize: true,
  //  //mathjax: true,
  //  mark: true,
  //  //mermaid: true,
  //  tabs: true,
  //  tasklist: true,
  //  vPre: true,
  //  component: true,
  //
  //  highlighter: {
  //    'type': 'shiki',
  //    themes: {
  //      light: "light-plus",
  //      dark: "nord",
  //    }
  //  },
  //},

  //plugins: {
  //  activeHeaderLinks: false,
  //
  //  comment: {
  //    provider: "Giscus",
  //    repo: "MaaAssistantArknights/maa-website",
  //    repoId: "R_kgDOHY7Gyg",
  //    category: "Comments",
  //    categoryId: "DIC_kwDOHY7Gys4CgoVH",
  //    mapping: "pathname",
  //    strict: true,
  //  },
  //
  //  icon: {
  //    assets: "iconify",
  //  },
  //
  //  sitemap: true,
  //  seo: true,
  //
  //},
});
