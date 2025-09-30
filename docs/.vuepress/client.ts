import { defineClientConfig } from 'vuepress/client';

import AsciiArt from './components/AsciiArt.vue';
import ImageGrid from './components/ImageGrid.vue';

import './styles/index.scss';

export default defineClientConfig({
  enhance: ({ app }) => {
    app.component('AsciiArt', AsciiArt);
    app.component('ImageGrid', ImageGrid);
  },
});
