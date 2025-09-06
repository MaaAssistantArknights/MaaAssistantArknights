import { defineNoteConfig, defineNotesConfig } from "vuepress-theme-plume";

export const enusNotes = defineNotesConfig({
  dir: "en-us",
  link: "/en-us/",
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
