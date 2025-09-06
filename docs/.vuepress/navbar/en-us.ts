import { defineNavbarConfig } from "vuepress-theme-plume";

export const enusNavbar = defineNavbarConfig([
  {
    text: "User Manual",
    icon: "mdi:user",
    link: "/en-us/manual/",
  },
  {
    text: "Development Docs",
    icon: "ph:code-bold",
    link: "/en-us/develop/",
  },
  {
    text: "Protocol Docs",
    icon: "basil:document-solid",
    link: "/en-us/protocol/",
  },
]);
