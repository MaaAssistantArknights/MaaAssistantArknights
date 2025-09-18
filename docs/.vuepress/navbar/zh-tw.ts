import { defineNavbarConfig } from "vuepress-theme-plume";

export const zhtwNavbar = defineNavbarConfig([
  {
    text: "用戶說明書",
    icon: "mdi:user",
    link: "/zh-tw/manual/",
  },
  {
    text: "開發文件",
    icon: "ph:code-bold",
    link: "/zh-tw/develop/",
  },
  {
    text: "協議文件",
    icon: "basil:document-solid",
    link: "/zh-tw/protocol/",
  },
]);
