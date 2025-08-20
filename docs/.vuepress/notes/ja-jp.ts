import { defineNoteConfig, defineNotesConfig } from "vuepress-theme-plume";

export const jajpNotes = defineNotesConfig({
  dir: "ja-jp",
  link: "/ja-jp/",
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
