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
      children: "structure",
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
