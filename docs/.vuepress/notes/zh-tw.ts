import { defineNotesConfig } from "vuepress-theme-plume"
import { genNotes } from "./genSidebar"

export const zhtwNotes = defineNotesConfig({
  dir: "zh-tw",
  link: "/zh-tw/",
  notes: genNotes("zh-tw"),
})
