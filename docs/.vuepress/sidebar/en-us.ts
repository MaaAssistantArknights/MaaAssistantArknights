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
          text: "Beginner's guide",
          icon: "ri:guide-fill",
          link: "/en-us/newbie",
        },
        {
          text: "Introduction",
          icon: "mdi:information-outline",
          link: "/en-us/introduction",
        },
        {
          text: "FAQs",
          icon: "ph:question-fill",
          link: "/en-us/faq",
        },
        {
          text: "Emulator Supports",
          icon: "mingcute:laptop-fill",
          collapsible: true,
          children: "structure",
        },
        {
          text: "User Manual For CLI",
          icon: "material-symbols:terminal",
          prefix: "cli/",
          collapsible: true,
          children: "structure",
        }
      ],
    },
    {
      text: "Development Docs",
      icon: "ph:code-bold",
      collapsible: true,
      children: [
        {
          text: "Linux Compilation Tutorial",
          icon: "teenyicons:linux-alt-solid",
          link: "/en-us/2.1-LINUX_TUTORIAL",
        },
        {
          text: "Development",
          icon: "iconoir:developer",
          link: "/en-us/2.2-DEVELOPMENT",
        },
        {
          text: "IssueBot Usage",
          icon: "bxs:bot",
          link: "/en-us/2.3-ISSUE_BOT_USAGE",
        },
        {
          text: "Web PR Tutorial",
          icon: "mingcute:git-pull-request-fill",
          link: "/en-us/2.4-PURE_WEB_PR_TUTORIAL",
        },
        {
          text: "Overseas Clients Adaptation",
          icon: "ri:earth-fill",
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
          icon: "bxs:book",
          link: "/en-us/3.1-INTEGRATION",
        },
        {
          text: "Callback Schema",
          icon: "material-symbols:u-turn-left",
          link: "/en-us/3.2-CALLBACK_SCHEMA",
        },
        {
          text: "Copilot Schema",
          icon: "ph:sword-bold",
          link: "/en-us/3.3-COPILOT_SCHEMA",
        },
        {
          text: "Task Schema",
          icon: "material-symbols:task",
          link: "/en-us/3.4-TASK_SCHEMA",
        },
        {
          text: "Integrated Strategy Schema",
          icon: "ri:game-fill",
          link: "/en-us/3.5-INTEGRATED_STRATEGY_SCHEMA",
        },
        {
          text: "Infrastructure Scheduling Schema",
          icon: "material-symbols:view-quilt-rounded",
          link: "/en-us/3.6-INFRASTRUCTURE_SCHEDULING_SCHEMA",
        },
        {
          text: "Security Presence Schema",
          icon: "game-icons:prisoner",
          link: "/en-us/3.7-SECURITY_PRESENCE_SCHEMA",
        },
      ],
    },
  ],
});
