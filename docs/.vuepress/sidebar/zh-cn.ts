import { sidebar } from "vuepress-theme-hope";

export const zhcnSidebar = sidebar({
  "/zh-cn/": [
    {
      text: "MAA",
      icon: "ic:round-home",
      link: "/zh-cn/",
    },
    {
      text: "用户手册",
      icon: "mdi:user",
      prefix: "manual/",
      collapsible: true,
      children: "structure",
    },
    {
      text: "开发文档",
      icon: "ph:code-bold",
      prefix: "develop/",
      collapsible: true,
      children: "structure",
    },
    {
      text: "协议文档",
      icon: "basil:document-solid",
      prefix: "protocol/",
      collapsible: true,
      children: "structure",
    },
  ],
});
