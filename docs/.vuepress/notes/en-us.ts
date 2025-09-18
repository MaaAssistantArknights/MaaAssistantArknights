import { defineNotesConfig } from 'vuepress-theme-plume';
import { genNotes } from './genSidebar';

export const enusNotes = defineNotesConfig({
  dir: 'en-us',
  link: '/en-us/',
  notes: genNotes('en-us'),
});
