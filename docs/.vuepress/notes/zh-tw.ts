import { defineNoteConfig, defineNotesConfig } from "vuepress-theme-plume";

export const zhtwNotes = defineNotesConfig({
  dir: "zh-tw",
  link: "/zh-tw/",
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
