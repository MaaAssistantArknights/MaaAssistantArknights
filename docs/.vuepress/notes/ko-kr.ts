import { defineNoteConfig, defineNotesConfig } from "vuepress-theme-plume";

export const kokrNotes = defineNotesConfig({
  dir: "ko-kr",
  link: "/ko-kr/",
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
