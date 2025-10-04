import { defineNotesConfig } from 'vuepress-theme-plume'
import { genNotes } from './genSidebar'

export const zhcnNotes = defineNotesConfig({
  dir: 'zh-cn',
  link: '/zh-cn/',
  notes: genNotes('zh-cn'),
})
