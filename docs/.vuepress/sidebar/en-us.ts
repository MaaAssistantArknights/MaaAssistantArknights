import { sidebar } from "vuepress-theme-hope";

export const enusSidebar = sidebar({
  "/en-us/": [
    {
      text: "MAA",
      icon: "ic:round-home",
      link: "/en-us/",
    },
    {
      text: "User Manual",
      icon: "mdi:user",
      collapsible: true,
      children: [
        {
          text: "Introduction",
          link: "/en-us/1.1-USER_MANUAL",
        },
        {
          text: "FAQs",
          link: "/en-us/1.2-FAQ",
        },
        {
          text: "Emulator Supports",
          icon: "laptop-code",
          collapsible: true,
          children: [
            {
              text: "Windows",
              link: "/en-us/1.3-EMULATOR_SUPPORTS",
            },
            {
              text: "Mac",
              link: "/en-us/1.4-EMULATOR_SUPPORTS_FOR_MAC",
            },
            {
              text: "Linux",
              link: "/en-us/1.5-EMULATOR_SUPPORTS_FOR_LINUX",
            },
          ],
        },
      ],
    },
    {
      text: "Development Docs",
      icon: "ph:code-bold",
      collapsible: true,
      children: [
        {
          text: "Linux Compilation Tutorial",
          link: "/en-us/2.1-LINUX_TUTORIAL",
        },
        {
          text: "Development",
          link: "/en-us/2.2-DEVELOPMENT",
        },
        {
          text: "IssueBot Usage",
          link: "/en-us/2.3-ISSUE_BOT_USAGE",
        },
        {
          text: "Web PR Tutorial",
          link: "/en-us/2.4-PURE_WEB_PR_TUTORIAL",
        },
        {
          text: "Overseas Clients Adaptation",
          link: "/en-us/2.5-OVERSEAS_CLIENTS_ADAPTATION",
        },
      ],
    },
    {
      text: "Protocol Docs",
      icon: "basil:document-solid",
      collapsible: true,
      children: [
        {
          text: "Integration",
          link: "/en-us/3.1-INTEGRATION",
        },
        {
          text: "Callback Schema",
          link: "/en-us/3.2-CALLBACK_SCHEMA",
        },
        {
          text: "Copilot Schema",
          link: "/en-us/3.3-COPILOT_SCHEMA",
        },
        {
          text: "Task Schema",
          link: "/en-us/3.4-TASK_SCHEMA",
        },
        {
          text: "Integrated Strategy Schema",
          link: "/en-us/3.5-INTEGRATED_STRATEGY_SCHEMA",
        },
        {
          text: "Infrastructure Scheduling Schema",
          link: "/en-us/3.6-INFRASTRUCTURE_SCHEDULING_SCHEMA",
        },
        {
          text: "Security Presence Schema",
          link: "/en-us/3.7-SECURITY_PRESENCE_SCHEMA",
        },
      ],
    },
  ],
});
