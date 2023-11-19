import { navbar } from "vuepress-theme-hope";

export const zhtwNavbar = navbar([
  {
    text: "首頁",
    icon: "ic:round-home",
    link: "/zh-tw/",
  },
  {
    text: "用戶說明書",
    icon: "mdi:user",
    link: "/zh-tw/1.1-詳細介紹",
  },
  {
    text: "開發文件",
    icon: "ph:code-bold",
    link: "/zh-tw/2.1-Linux編譯教學",
  },
  {
    text: "協議文件",
    icon: "basil:document-solid",
    link: "/zh-tw/3.1-集成文件",
  },
  {
    text: "MAA 官網",
    icon: "mdi:cow",
    link: "https://maa.plus",
  },
]);
