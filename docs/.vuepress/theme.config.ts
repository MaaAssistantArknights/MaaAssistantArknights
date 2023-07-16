import { defaultTheme } from '@vuepress/theme-default';

export default defaultTheme({
  repo: 'MaaAssistantArknights/MaaAssistantArknights',
  sidebarDepth: 2,
  docsBranch: 'dev',
  docsDir: 'docs',
  editLinkPattern: ':repo/edit/:branch/:path',
  navbar: [
    // { text: "必读", link: "/guide.html" },
    { text: "MAA 官网", link: "/", target: "_blank" }
  ],
  locales: {
    '/': {
      selectLanguageName: '简体中文',
      home: '/',
      sidebar: {
        '/': [
          '1.1-详细介绍.html',
          '1.2-常见问题.html',
          '1.3-模拟器支持.html',
          '1.4-Mac模拟器支持.html',
          '1.5-Linux模拟器支持.html',
          '2.1-Linux编译教程.html',
          '2.2-开发相关.html',
          '2.3-IssueBot使用方法.html',
          '2.4-纯网页端PR教程.html',
          '2.5-外服适配教程.html',
          '3.1-集成文档.html',
          '3.2-回调消息协议.html',
          '3.3-战斗流程协议.html',
          '3.4-任务流程协议.html',
          '3.5-肉鸽辅助协议.html',
          '3.6-基建排班协议.html',
          '3.7-保全派驻协议.html',
        ],
      },
    },
    '/zh-tw/': {
      selectLanguageName: '繁體中文',
      home: '/zh-tw/',
      sidebar: {
        '/zh-tw/': [
          '1.1-詳細介紹.html',
          '1.2-常見問題.html',
          '1.3-模擬器支援.html',
          '1.4-Mac模擬器支援.html',
          '2.1-Linux編譯教程.html',
          '2.2-開發相關.html',
          '2.3-IssueBot使用方法.html',
          '2.4-純網頁端PR教程.html',
          '2.5-外服適配教程.html',
          '3.1-集成文件.html',
          '3.2-回呼訊息協定.html',
          '3.3-戰鬥流程協定.html',
          '3.4-任務流程協定.html',
          '3.5-肉鸽輔助協定.html',
          '3.6-基建排班協議.html',
          '3.7-保全派駐協議.html',
        ],
      },
    },
    '/en-us/': {
      selectLanguageName: 'English',
      home: '/en-us/',
      sidebar: {
        '/en-us/': [
          '1.1-USER_MANUAL.html',
          '1.2-FAQ.html',
          '1.3-EMULATOR_SUPPORTS.html',
          '1.4-EMULATOR_SUPPORTS_FOR_MAC.md',
          '1.5-EMULATOR_SUPPORTS_FOR_LINUX.html',
          '2.1-LINUX_TUTORIAL.html',
          '2.2-DEVELOPMENT.html',
          '2.3-ISSUE_BOT_USAGE.html',
          '2.4-PURE_WEB_PR_TUTORIAL.html',
          '2.5-OVERSEAS_CLIENTS_ADAPTATION.html',
          '3.1-INTEGRATION.html',
          '3.2-CALLBACK_SCHEMA.html',
          '3.3-COPILOT_SCHEMA.html',
          '3.4-TASK_SCHEMA.html',
          '3.5-INTEGRATED_STRATEGY_SCHEMA.html',
          '3.6-INFRASTRUCTURE_SCHEDULING_SCHEMA.html',
          '3.7-SECURITY_PRESENCE_SCHEMA.html',
        ],
      },
    },
    '/ja-jp/': {
      selectLanguageName: '日本語',
      home: '/ja-jp/',
      sidebar: {
        '/ja-jp/': [
          '1.1-詳細説明.html',
          '1.2-よくある質問.html',
          '1.3-エミュレータのサポート.html',
          '2.1-Linuxチュートリアル.html',
          '2.2-プルリクエスト.html',
          '2.3-IssueBotの使用方法.html',
          '2.4-純WebサイトのPRチュートリアル.html',
          '2.5-OVERSEAS_CLIENTS_ADAPTATION.html',
          '3.1-統合ドキュメント.html',
          '3.2-コールバックAPI.html',
          '3.3-自動作戦API.html',
          '3.4-タスクAPI.html',
          '3.5-統合戦略API.html',
          '3.6-インフラスケジュール設定.html',
          '3.7-SECURITY_PRESENCE_SCHEMA.html',
        ],
      },
    },
    '/ko-kr/': {
      selectLanguageName: '한국어',
      home: '/ko-kr/',
      sidebar: {
        '/ko-kr/': [
          '1.1-사용자_설명서.html',
          '1.2-FAQ.html',
          '1.3-에뮬레이터_지원.html',
        ]
      }
    }
  },
});
