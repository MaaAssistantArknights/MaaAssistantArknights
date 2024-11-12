import { navbar } from "vuepress-theme-hope";

export const zhcnNavbar = navbar([
  {
    text: "主页",
    icon: "ic:round-home",
    link: "/zh-cn/",
  },
  {
    text: "用户手册",
    icon: "mdi:user",
    link: "/zh-cn/manual/",
  },
  {
    text: "开发文档",
    icon: "ph:code-bold",
    link: "/zh-cn/develop/",
  },
  {
    text: "协议文档",
    icon: "basil:document-solid",
    link: "/zh-cn/protocol/",
  },
  {
    text: "MAA 官网",
    icon: "mdi:cow",
    link: "https://maa.plus",
  },
]);
