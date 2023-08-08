import { navbar } from "vuepress-theme-hope";

export const zhcnNavbar = navbar([
  {
    text: "主页",
    icon: "ic:round-home",
    link: "/",
  },
  {
    text: "用户手册",
    icon: "mdi:user",
    link: "/1.1-详细介绍",
  },
  {
    text: "开发相关",
    icon: "ph:code-bold",
    link: "/2.1-Linux编译教程",
  },
  {
    text: "协议文档",
    icon: "basil:document-solid",
    link: "/3.1-集成文档",
  },
  {
    text: "MAA 官网",
    icon: "mdi:cow",
    link: "https://maa.plus",
  },
]);
