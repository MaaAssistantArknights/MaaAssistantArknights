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
          icon: "mdi:information-outline",
          link: "/1.1-详细介绍",
        },
        {
          text: "常见问题",
          icon: "ph:question-fill",
          link: "/1.2-常见问题",
        },
        {
          text: "模拟器支持",
          icon: "mingcute:laptop-fill",
          collapsible: true,
          children: [
            {
              text: "Windows",
              icon: "ri:windows-fill",
              link: "/1.3-模拟器支持",
            },
            {
              text: "Mac",
              icon: "basil:apple-solid",
              link: "/1.4-Mac模拟器支持",
            },
            {
              text: "Linux",
              icon: "teenyicons:linux-alt-solid",
              link: "/1.5-Linux模拟器支持",
            },
          ],
        },
        {
          text: "CLI使用指南",
          icon: "material-symbols:terminal",
          link: "/1.6-CLI使用指南",
        },
      ],
    },
    {
      text: "开发文档",
      icon: "ph:code-bold",
      collapsible: true,
      children: [
        {
          text: "Linux编译教程",
          icon: "teenyicons:linux-alt-solid",
          link: "/2.1-Linux编译教程",
        },
        {
          text: "开发相关",
          icon: "iconoir:developer",
          link: "/2.2-开发相关",
        },
        {
          text: "IssueBot使用方法",
          icon: "bxs:bot",
          link: "/2.3-IssueBot使用方法",
        },
        {
          text: "纯网页端PR教程",
          icon: "mingcute:git-pull-request-fill",
          link: "/2.4-纯网页端PR教程",
        },
        {
          text: "外服适配教程",
          icon: "ri:earth-fill",
          link: "/2.5-外服适配教程",
        },
        {
          text: "文档编写指南",
          icon: "jam:write-f",
          link: "/2.6-文档编写指南",
        }
      ],
    },
    {
      text: "协议文档",
      icon: "basil:document-solid",
      collapsible: true,
      children: [
        {
          text: "集成文档",
          icon: "bxs:book",
          link: "/3.1-集成文档",
        },
        {
          text: "回调消息协议",
          icon: "material-symbols:u-turn-left",
          link: "/3.2-回调消息协议",
        },
        {
          text: "战斗流程协议",
          icon: "ph:sword-bold",
          link: "/3.3-战斗流程协议",
        },
        {
          text: "任务流程协议",
          icon: "material-symbols:task",
          link: "/3.4-任务流程协议",
        },
        {
          text: "肉鸽辅助协议",
          icon: "ri:game-fill",
          link: "/3.5-肉鸽辅助协议",
        },
        {
          text: "基建排班协议",
          icon: "material-symbols:view-quilt-rounded",
          link: "/3.6-基建排班协议",
        },
        {
          text: "保全派驻协议",
          icon: "game-icons:prisoner",
          link: "/3.7-保全派驻协议",
        },
        {
          text: "远程控制协议",
          icon: "mdi:remote-desktop",
          link: "/3.8-远程控制协议"
        },
      ],
    },
  ],
});
