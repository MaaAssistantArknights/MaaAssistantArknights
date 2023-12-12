import { sidebar } from "vuepress-theme-hope";

export const zhcnSidebar = sidebar({
  "/": [
    {
      text: "MAA",
      icon: "ic:round-home",
      link: "/",
    },
    {
      text: "用户手册",
      icon: "mdi:user",
      prefix: "用户手册/",
      collapsible: true,
      children: [
        {
          text: "功能介绍",
          icon: "mdi:information-outline",
          link: "详细介绍",
        },
        {
          text: "常见问题",
          icon: "ph:question-fill",
          link: "常见问题",
        },
        {
          text: "模拟器和设备支持",
          icon: "mingcute:laptop-fill",
          prefix: "模拟器和设备支持/",
          collapsible: true,
          children: "structure",
        },
        {
          text: "CLI使用指南",
          icon: "material-symbols:terminal",
          link: "CLI使用指南",
        },
      ],
    },
    {
      text: "开发文档",
      icon: "ph:code-bold",
      prefix: "开发文档/",
      collapsible: true,
      children: "structure",
    },
    {
      text: "协议文档",
      icon: "basil:document-solid",
      prefix: "协议文档/",
      collapsible: true,
      children: "structure",
    },
  ],
});
