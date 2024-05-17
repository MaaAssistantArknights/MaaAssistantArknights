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
      collapsible: true,
      children: [
        {
          text: "功能介绍",
          icon: "mdi:information-outline",
          link: "/zh-tw/1.1-詳細介紹",
        },
        {
          text: "常見問題",
          icon: "ph:question-fill",
          link: "/zh-tw/1.2-常見問題",
        },
        {
          text: "模擬器支援",
          icon: "mingcute:laptop-fill",
          collapsible: true,
          children: [
            {
              text: "Windows",
              icon: "ri:windows-fill",
              link: "/zh-tw/1.3-模擬器支援",
            },
            {
              text: "Mac",
              icon: "basil:apple-solid",
              link: "/zh-tw/1.4-Mac模擬器支援",
            },
            {
              text: "Linux",
              icon: "teenyicons:linux-alt-solid",
              link: "/zh-tw/1.5-Linux模擬器支援",
            },
          ],
        },
        {
          text: "CLI使用說明",
          icon: "material-symbols:terminal",
          link: "/zh-tw/1.6-CLI使用說明",
        },
      ],
    },
    {
      text: "開發文件",
      icon: "ph:code-bold",
      collapsible: true,
      children: [
        {
          text: "Linux編譯教學",
          icon: "teenyicons:linux-alt-solid",
          link: "/zh-tw/2.1-Linux編譯教學",
        },
        {
          text: "開發相關",
          icon: "iconoir:developer",
          link: "/zh-tw/2.2-開發相關",
        },
        {
          text: "IssueBot使用方法",
          icon: "bxs:bot",
          link: "/zh-tw/2.3-IssueBot使用方法",
        },
        {
          text: "純網頁端PR教學",
          icon: "mingcute:git-pull-request-fill",
          link: "/zh-tw/2.4-純網頁端PR教學",
        },
        {
          text: "外服適配教學",
          icon: "ri:earth-fill",
          link: "/zh-tw/2.5-外服適配教學",
        },
      ],
    },
    {
      text: "協議文件",
      icon: "basil:document-solid",
      collapsible: true,
      children: [
        {
          text: "集成文件",
          icon: "bxs:book",
          link: "/zh-tw/3.1-集成文件",
        },
        {
          text: "回呼訊息協議",
          icon: "material-symbols:u-turn-left",
          link: "/zh-tw/3.2-回呼訊息協議",
        },
        {
          text: "戰鬥流程協議",
          icon: "ph:sword-bold",
          link: "/zh-tw/3.3-戰鬥流程協議",
        },
        {
          text: "任務流程協議",
          icon: "material-symbols:task",
          link: "/zh-tw/3.4-任務流程協議",
        },
        {
          text: "肉鴿輔助協議",
          icon: "ri:game-fill",
          link: "/zh-tw/3.5-肉鴿輔助協議",
        },
        {
          text: "基建排班協議",
          icon: "material-symbols:view-quilt-rounded",
          link: "/zh-tw/3.6-基建排班協議",
        },
        {
          text: "保全派駐協議",
          icon: "game-icons:prisoner",
          link: "/zh-tw/3.7-保全派駐協議",
        },
      ],
    },
  ],
});
