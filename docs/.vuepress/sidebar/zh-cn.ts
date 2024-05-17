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
      children: [
        {
          text: "新手上路",
          icon: "ri:guide-fill",
          link: "newbie",
        },
        {
          text: "功能介绍",
          icon: "mdi:information-outline",
          link: "introduction",
        },
        {
          text: "常见问题",
          icon: "ph:question-fill",
          link: "faq",
        },
        {
          text: "自定义连接",
          icon: "mdi:plug",
          link: "custom-connection",
        },
        {
          text: "模拟器和设备支持",
          icon: "mingcute:laptop-fill",
          prefix: "devices/",
          collapsible: true,
          children: "structure",
        },
        {
          text: "CLI 使用指南",
          icon: "material-symbols:terminal",
          prefix: "cli/",
          collapsible: true,
          children: "structure",
        },
      ],
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
