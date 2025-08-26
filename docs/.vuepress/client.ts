import { defineClientConfig } from "vuepress/client";

import ImageGrid from "./components/ImageGrid.vue";

import './styles/index.scss'

export default defineClientConfig({
  enhance: ({ app }) => {
    app.component("ImageGrid", ImageGrid);
  },
});
