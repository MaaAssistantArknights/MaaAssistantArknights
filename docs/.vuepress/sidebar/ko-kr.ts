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
      collapsible: true,
      children: [
        {
          text: "기능",
          icon: "mdi:information-outline",
          link: "/ko-kr/1.1-사용자_설명서",
        },
        {
          text: "자주 묻는 질문",
          icon: "ph:question-fill",
          link: "/ko-kr/1.2-FAQ",
        },
        {
          text: "에뮬레이터 지원",
          icon: "mingcute:laptop-fill",
          collapsible: true,
          children: [
            {
              text: "Windows",
              icon: "ri:windows-fill",
              link: "/ko-kr/1.3-에뮬레이터_지원",
            },
            {
              text: "Mac",
              icon: "basil:apple-solid",
              link: "/ko-kr/1.4-Mac에뮬레이터_지원",
            },
            {
              text: "Linux",
              icon: "teenyicons:linux-alt-solid",
              link: "/ko-kr/1.5-LINUX용_에뮬레이터 지원",
            },
          ],
        },
      ],
    },
    {
      text: "개발 관련",
      icon: "ph:code-bold",
      collapsible: true,
      children: [
        {
          text: "리눅스 컴파일 튜토리얼",
          icon: "teenyicons:linux-alt-solid",
          link: "/ko-kr/2.1-LINUX_튜토리얼",
        },
        {
          text: "개발 환경 구축하기",
          icon: "iconoir:developer",
          link: "/ko-kr/2.2-개발_환경_구축하기",
        },
        {
          text: "Issue Bot 사용 방법",
          icon: "bxs:bot",
          link: "/ko-kr/2.3-Issue_Bot_사용방법",
        },
        {
          text: "GitHub Pull Request 사용 가이드",
          icon: "mingcute:git-pull-request-fill",
          link: "/ko-kr/2.4-웹을_통한_풀리퀘스트_튜토리얼",
        },
        {
          text: "해외 클라이언트 현지화",
          icon: "ri:earth-fill",
          link: "/ko-kr/2.5-해외_클라이언트_현지화",
        },
      ],
    },
    {
      text: "프로토콜 문서",
      icon: "basil:document-solid",
      collapsible: true,
      children: [
        {
          text: "연동",
          icon: "bxs:book",
          link: "/ko-kr/3.1-연동",
        },
        {
          text: "콜백 스키마",
          icon: "material-symbols:u-turn-left",
          link: "/ko-kr/3.2-콜백_스키마",
        },
        {
          text: "전투 스키마",
          icon: "ph:sword-bold",
          link: "/ko-kr/3.3-전투_스키마",
        },
        {
          text: "작업 스키마",
          icon: "material-symbols:task",
          link: "/ko-kr/3.4-작업_스키마",
        },
        {
          text: "통합전략 스키마",
          icon: "ri:game-fill",
          link: "/ko-kr/3.5-통합전략_스키마",
        },
        {
          text: "기반시설 예약 스키마",
          icon: "material-symbols:view-quilt-rounded",
          link: "/ko-kr/3.6-기반시설_예약_스키마",
        },
        {
          text: "보안파견 스키마",
          icon: "game-icons:prisoner",
          link: "/ko-kr/3.7-보안파견_스키마",
        },
      ],
    },
  ],
});
