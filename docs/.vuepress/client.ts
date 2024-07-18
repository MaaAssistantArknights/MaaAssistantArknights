import { defineClientConfig } from "vuepress/client";
import ImageGrid from "../components/ImageGrid.vue";

export default defineClientConfig({
  enhance: ({ app }) => {
    app.component("ImageGrid", ImageGrid);
  },
});
