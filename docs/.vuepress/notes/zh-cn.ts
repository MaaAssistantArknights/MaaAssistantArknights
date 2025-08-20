import { defineNoteConfig, defineNotesConfig } from "vuepress-theme-plume";

export const zhcnNotes = defineNotesConfig({
  dir: "zh-cn",
  link: "/zh-cn/",
  notes: [
    defineNoteConfig({
      dir: "manual",
      link: "/manual/",
      sidebar: "auto",
    }),
    defineNoteConfig({
      dir: "develop",
      link: "/develop/",
      sidebar: "auto",
    }),
    defineNoteConfig({
      dir: "protocol",
      link: "/protocol/",
      sidebar: "auto",
    }),
    
  ]
});