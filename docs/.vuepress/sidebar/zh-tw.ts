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
      children: [
        {
          text: "新手上路(translation required)",
          icon: "ri:guide-fill",
          link: "newbie",
        },
        {
          text: "功能介绍",
          icon: "mdi:information-outline",
          link: "introduction",
        },
        {
          text: "连接设置(translation required)",
          icon: "mdi:plug",
          link: "connection",
        },
        {
          text: "常見問題",
          icon: "ph:question-fill",
          link: "faq",
        },
        {
          text: "模拟器和设备支持(translation required)",
          icon: "mingcute:laptop-fill",
          prefix: "devices/",
          collapsible: true,
          children: "structure",
        },
        {
          text: "CLI 使用說明",
          icon: "material-symbols:terminal",
          prefix: "cli/",
          collapsible: true,
          children: "structure",
        },
      ],
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
