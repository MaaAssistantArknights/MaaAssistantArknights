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
          text: "플랫폼 별 지원",
          icon: "mingcute:laptop-fill",
          prefix: "플랫폼/",
          collapsible: true,
          children: [
            {
              text: "Windows 지원",
              icon: "ri:windows-fill",
              link: "1.Windows",
            },
            {
              text: "Mac 지원",
              icon: "basil:apple-solid",
              link: "2.Mac",
            },
            {
              text: "Linux 지원",
              icon: "teenyicons:linux-alt-solid",
              link: "3.Linux",
            },
            {
              text: "Android 지원",
              icon: "mingcute:android-fill",
              link: "4.Android",
            },
          ],
        },
        {
          text: "CLI 가이드",
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
      children: [
        {
          text: "개발 환경 구축",
          icon: "iconoir:developer",
          link: "1.개발시작",
        },
        {
          text: "Linux 튜토리얼",
          icon: "teenyicons:linux-alt-solid",
          link: "2.Linux가이드",
        },
        {
          text: "웹 기반 PR 가이드",
          icon: "mingcute:git-pull-request-fill",
          link: "3.웹기반PR",
        },
        {
          text: "IssueBot 사용방법",
          icon: "bxs:bot",
          link: "4.IssueBot",
        },
        {
          text: "해외 클라이언트 현지화",
          icon: "ri:earth-fill",
          link: "5.해외클라이언트현지화",
        },
        {
          text: "문서 작성 가이드",
          icon: "jam:write-f",
          link: "6.문서가이드",
        },
      ],
    },
    {
      text: "프로토콜 문서",
      icon: "basil:document-solid",
      prefix: "스키마/",
      collapsible: true,
      children: [
        {
          text: "통합문서",
          icon: "bxs:book",
          link: "1.통합문서",
        },
        {
          text: "콜백 스키마",
          icon: "material-symbols:u-turn-left",
          link: "2.콜백",
        },
        {
          text: "전투 스키마",
          icon: "ph:sword-bold",
          link: "3.전투",
        },
        {
          text: "작업 스키마",
          icon: "material-symbols:task",
          link: "4.작업",
        },
        {
          text: "통합전략 스키마",
          icon: "ri:game-fill",
          link: "5.통합전략",
        },
        {
          text: "기반시설 스키마",
          icon: "material-symbols:view-quilt-rounded",
          link: "6.기반시설",
        },
        {
          text: "보안파견 스키마",
          icon: "game-icons:prisoner",
          link: "7.보안파견",
        },
        {
          text: "원격제어 스키마",
          icon: "mdi:remote-desktop",
          link: "8.원격제어",
        },
      ],
    },
  ],
});
