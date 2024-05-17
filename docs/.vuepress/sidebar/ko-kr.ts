import { sidebar } from "vuepress-theme-hope";

export const kokrSidebar = sidebar({
  "/ko-kr/": [
    {
      text: "MAA",
      icon: "ic:round-home",
      link: "/ko-kr/",
    },
    {
      text: "사용자 설명서",
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
          text: "기능",
          icon: "mdi:information-outline",
          link: "introduction",
        },
        {
          text: "连接设置(translation required)",
          icon: "mdi:plug",
          link: "connection",
        },
        {
          text: "자주 묻는 질문",
          icon: "ph:question-fill",
          link: "faq",
        },
        {
          text: "플랫폼 별 지원(translation required)",
          icon: "mingcute:laptop-fill",
          prefix: "devices/",
          collapsible: true,
          children: "structure",
        },
        {
          text: "CLI 가이드",
          icon: "material-symbols:terminal",
          prefix: "cli/",
          collapsible: true,
          children: "structure",
        },
      ],
    },
    {
      text: "개발 관련",
      icon: "ph:code-bold",
      prefix: "develop/",
      collapsible: true,
      children: "structure",
    },
    {
      text: "프로토콜 문서",
      icon: "basil:document-solid",
      prefix: "protocol/",
      collapsible: true,
      children: "structure",
    },
  ],
});
