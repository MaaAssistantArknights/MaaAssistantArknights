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
          text: "초보자 가이드",
          icon: "ri:guide-fill",
          link: "newbie",
        },
        {
          text: "기능",
          icon: "mdi:information-outline",
          prefix: "introduction/",
          collapsible: true,
          children: "structure",
        },
        {
          text: "연결 설정",
          icon: "mdi:plug",
          link: "connection",
        },
        {
          text: "자주 묻는 질문",
          icon: "ph:question-fill",
          link: "faq",
        },
        {
          text: "플랫폼 별 지원",
          icon: "mingcute:laptop-fill",
          prefix: "device/",
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
