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
      collapsible: true,
      children: [
        {
          text: "功能介绍",
          link: "/1.1-详细介绍",
        },
        {
          text: "常见问题",
          link: "/1.2-常见问题",
        },
        {
          text: "模拟器支持",
          icon: "laptop-code",
          collapsible: true,
          children: [
            {
              text: "Windows",
              link: "/1.3-模拟器支持",
            },
            {
              text: "Mac",
              link: "/1.4-Mac模拟器支持",
            },
            {
              text: "Linux",
              link: "/1.5-Linux模拟器支持",
            },
          ],
        },
      ],
    },
    {
      text: "开发相关",
      icon: "ph:code-bold",
      collapsible: true,
      children: [
        {
          text: "Linux编译教程",
          link: "/2.1-Linux编译教程",
        },
        {
          text: "本地PR教程及clang-format配置",
          link: "/2.2-开发相关",
        },
        {
          text: "Github IssueBot使用方法",
          link: "/2.3-IssueBot使用方法",
        },
        {
          text: "纯网页端PR教程",
          link: "/2.4-纯网页端PR教程",
        },
        {
          text: "外服适配教程",
          link: "/2.5-外服适配教程",
        },
      ],
    },
    {
      text: "协议文档",
      icon: "basil:document-solid",
      collapsible: true,
      children: [
        {
          text: "集成文档",
          link: "/3.1-集成文档",
        },
        {
          text: "回调消息协议",
          link: "/3.2-回调消息协议",
        },
        {
          text: "战斗流程协议",
          link: "/3.3-战斗流程协议",
        },
        {
          text: "任务流程协议",
          link: "/3.4-任务流程协议",
        },
        {
          text: "肉鸽辅助协议",
          link: "/3.5-肉鸽辅助协议",
        },
        {
          text: "基建排班协议",
          link: "/3.6-基建排班协议",
        },
        {
          text: "保全派驻协议",
          link: "/3.7-保全派驻协议",
        },
      ],
    },
  ],
});
