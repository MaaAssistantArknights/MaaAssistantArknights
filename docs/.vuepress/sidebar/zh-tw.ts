import { sidebar } from "vuepress-theme-hope";

export const zhtwSidebar = sidebar({
  "/zh-tw/": [
    {
      text: "MAA",
      icon: "ic:round-home",
      link: "/zh-tw/",
    },
    {
      text: "用户手册",
      icon: "mdi:user",
      collapsible: true,
      children: [
        {
          text: "功能介绍",
          link: "/zh-tw/1.1-詳細介紹",
        },
        {
          text: "常見問題",
          link: "/zh-tw/1.2-常見問題",
        },
        {
          text: "模擬器支援",
          icon: "laptop-code",
          collapsible: true,
          children: [
            {
              text: "Windows",
              link: "/zh-tw/1.3-模擬器支援",
            },
            {
              text: "Mac",
              link: "/zh-tw/1.4-Mac模擬器支援",
            },
          ],
        },
      ],
    },
    {
      text: "開發文檔",
      icon: "ph:code-bold",
      collapsible: true,
      children: [
        {
          text: "Linux編譯教程",
          link: "/zh-tw/2.1-Linux編譯教程",
        },
        {
          text: "开发相关",
          link: "/zh-tw/2.2-開發相關",
        },
        {
          text: "Github IssueBot使用方法",
          link: "/zh-tw/2.3-IssueBot使用方法",
        },
        {
          text: "純網頁端PR教程",
          link: "/zh-tw/2.4-純網頁端PR教程",
        },
        {
          text: "外服適配教程",
          link: "/zh-tw/2.5-外服適配教程",
        },
      ],
    },
    {
      text: "協定文檔",
      icon: "basil:document-solid",
      collapsible: true,
      children: [
        {
          text: "集成文件",
          link: "/zh-tw/3.1-集成文件",
        },
        {
          text: "回调消息协议",
          link: "/zh-tw/3.2-回呼訊息協定",
        },
        {
          text: "戰鬥流程協定",
          link: "/zh-tw/3.3-戰鬥流程協定",
        },
        {
          text: "任务流程协议",
          link: "/zh-tw/3.4-任務流程協定",
        },
        {
          text: "肉鸽輔助協定",
          link: "/zh-tw/3.5-肉鸽輔助協定",
        },
        {
          text: "基建排班协议",
          link: "/zh-tw/3.6-基建排班協議",
        },
        {
          text: "保全派駐協議",
          link: "/zh-tw/3.7-保全派駐協議",
        },
      ],
    },
  ],
});
