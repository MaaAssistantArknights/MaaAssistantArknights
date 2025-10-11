import { defineThemeConfig } from 'vuepress-theme-plume'
import { genThemeLocales } from './navigation/genLocales'

export default defineThemeConfig({
  logo: '/images/maa-logo_512x512.png',

  appearance: true,

  social: [
    { icon: 'qq', link: 'https://api.maa.plus/MaaAssistantArknights/api/qqgroup' },
    { icon: 'discord', link: 'https://discord.gg/23DfZ9uA4V' },
    { icon: 'telegram', link: 'https://t.me/+Mgc2Zngr-hs3ZjU1' },
    { icon: 'bilibili', link: 'https://space.bilibili.com/3493274731940507/' },
    { icon: 'github', link: 'https://github.com/MaaAssistantArknights/MaaAssistantArknights/' },
  ],
  navbarSocialInclude: ['qq', 'discord', 'telegram', 'bilibili', 'github'],

  aside: true,
  // outline: [2, 3, 4, 5],
  copyright: false,
  prevPage: false,
  nextPage: false,
  createTime: false,

  footer: false,

  locales: genThemeLocales(),

  autoFrontmatter: false,

  //plugins: {
  //  notice: [
  //    {
  //      path: "/zh-tw/",
  //      title: "翻译警告",
  //      content: "MAA 的文檔以簡體中文為主，其他語言的文檔可能品質低或尚未翻譯，請諒解。",
  //      fullscreen: true,
  //      confirm: true,
  //      showOnce: true,
  //      actions: [
  //        {
  //          text: "我知道了",
  //          type: "primary",
  //        },
  //        {
  //          text: "前往簡體中文",
  //          link: "/zh-cn/",
  //        },
  //      ],
  //    },
  //    {
  //      path: "/en-us/",
  //      title: "Translation Warning",
  //      content: "MAA's documents are mainly in Simplified Chinese. Documents in other languages may be of low quality or not yet translated. Please understand.",
  //      fullscreen: true,
  //      confirm: true,
  //      showOnce: true,
  //      actions: [
  //        {
  //          text: "Okay",
  //          type: "primary",
  //        },
  //        {
  //          text: "Take me to zh-CN",
  //          link: "/zh-cn/",
  //        },
  //      ],
  //    },
  //    {
  //      path: "/ja-jp/",
  //      title: "翻訳に関する警告",
  //      content: "MAA のドキュメントは主に簡体字中国語で書かれており、他の言語のドキュメントは低品質であるか、翻訳されていない可能性がありますので、ご了承ください。",
  //      fullscreen: true,
  //      confirm: true,
  //      showOnce: true,
  //      actions: [
  //        {
  //          text: "OK",
  //          type: "primary",
  //        },
  //        {
  //          text: "中国語サイトへ行く",
  //          link: "/zh-cn/",
  //        },
  //      ],
  //    },
  //    {
  //      path: "/ko-kr/",
  //      title: "번역 경고",
  //      content: "MAA의 문서는 주로 중국어 간체로 되어 있습니다. 다른 언어로 된 문서는 번역이 이상하거나, 번역이 되어있지 않을 수 있습니다.",
  //      fullscreen: true,
  //      confirm: true,
  //      showOnce: true,
  //      actions: [
  //        {
  //          text: "OK",
  //          type: "primary",
  //        },
  //        {
  //          text: "중국어 간체로 이동",
  //          link: "/zh-cn/",
  //        },
  //      ],
  //    },
  //  ],
  //},
})
