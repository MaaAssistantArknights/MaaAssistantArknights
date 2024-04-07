import { sidebar } from "vuepress-theme-hope";

export const zhcnSidebar = sidebar({
  "/ko-kr/": [
    {
      text: "MAA",
      icon: "ic:round-home",
      link: "/ko-kr/",
    },
    {
      text: "사용자 설명서",
      icon: "mdi:user",
      prefix: "사용자설명서/",
      collapsible: true,
      children: [
        {
          text: "기능",
          icon: "mdi:information-outline",
          link: "사용자설명서",
        },
        {
          text: "자주 묻는 질문",
          icon: "ph:question-fill",
          link: "FAQ",
        },
        {
          text: "플랫폼 별 지원 에뮬레이터",
          icon: "mingcute:laptop-fill",
          prefix: "플랫폼/",
          collapsible: true,
          children: "structure",
        },
        {
          text: "CLI使用指南",
          icon: "material-symbols:terminal",
          link: "CLI",
        },
      ],
    },
    {
      text: "개발 관련",
      icon: "ph:code-bold",
      prefix: "개발문서/",
      collapsible: true,
      children: "structure",
    },
    {
      text: "프로토콜 문서",
      icon: "basil:document-solid",
      prefix: "스키마/",
      collapsible: true,
      children: "structure",
    },
  ],
});
