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
      prefix: "user_manual/",
      collapsible: true,
      children: [
        {
          text: "Beginner's guide",
          icon: "ri:guide-fill",
          link: "BEGINNERS_GUIDE",
        },
        {
          text: "Introduction",
          icon: "mdi:information-outline",
          link: "USER_MANUAL",
        },
        {
          text: "FAQs",
          icon: "ph:question-fill",
          link: "FAQ",
        },
        {
          text: "Emulator Supports",
          icon: "mingcute:laptop-fill",
          prefix: "emulator_device_support/",
          collapsible: true,
          children: "structure",
        },
        {
          text: "User Manual For CLI",
          icon: "material-symbols:terminal",
          prefix: "user_manual_for_cli/",
          collapsible: true,
          children: "structure",
        }
      ],
    },
    {
      text: "Development Docs",
      icon: "ph:code-bold",
      prefix: "development_docs/",
      collapsible: true,
      children: "structure",
    },
    {
      text: "Protocol Docs",
      icon: "basil:document-solid",
      prefix: "protocol_docs/",
      collapsible: true,
      children: "structure",
    },
  ],
});
