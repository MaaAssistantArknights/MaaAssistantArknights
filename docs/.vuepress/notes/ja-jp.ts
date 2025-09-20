import { defineNotesConfig } from 'vuepress-theme-plume';
import { genNotes } from './genSidebar';

export const jajpNotes = defineNotesConfig({
  dir: 'ja-jp',
  link: '/ja-jp/',
  notes: genNotes('ja-jp'),
});
