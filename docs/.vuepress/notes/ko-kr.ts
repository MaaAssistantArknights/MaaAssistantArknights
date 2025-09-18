import { defineNotesConfig } from 'vuepress-theme-plume';
import { genNotes } from './genSidebar';

export const kokrNotes = defineNotesConfig({
  dir: 'ko-kr',
  link: '/ko-kr/',
  notes: genNotes('ko-kr'),
});
