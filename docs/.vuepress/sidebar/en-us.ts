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
      prefix: "manual/",
      collapsible: true,
      children: [
        {
          text: "Beginner's guide",
          icon: "ri:guide-fill",
          link: "newbie",
        },
        {
          text: "Introduction",
          icon: "mdi:information-outline",
          link: "introduction",
        },
        {
          text: "FAQs",
          icon: "ph:question-fill",
          link: "faq",
        },
        {
          text: "Custom Connection",
          icon: "mdi:plug",
          link: "custom-connection",
        },
        {
          text: "Emulator Supports",
          icon: "mingcute:laptop-fill",
          prefix: "devices/",
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
      prefix: "develop/",
      collapsible: true,
      children: "structure",
    },
    {
      text: "Protocol Docs",
      icon: "basil:document-solid",
      prefix: "protocol/",
      collapsible: true,
      children: "structure",
    },
  ],
});
