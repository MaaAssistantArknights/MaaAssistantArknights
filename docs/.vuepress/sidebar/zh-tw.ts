import { sidebar } from "vuepress-theme-hope";

export const zhtwSidebar = sidebar({
  "/zh-tw/": [
    {
      text: "MAA",
      icon: "ic:round-home",
      link: "/zh-tw/",
    },
    {
      text: "用戶說明書",
      icon: "mdi:user",
      prefix: "manual/",
      collapsible: true,
      children: "structure",
    },
    {
      text: "開發文件",
      icon: "ph:code-bold",
      prefix: "develop/",
      collapsible: true,
      children: "structure",
    },
    {
      text: "協議文件",
      icon: "basil:document-solid",
      prefix: "protocol/",
      collapsible: true,
      children: "structure",
    },
  ],
});
