import { defineClientConfig } from "vuepress/client";
import ImageGrid from "../components/ImageGrid.vue";
import Image from "../components/Image.vue";

export default defineClientConfig({
  enhance: ({ app }) => {
    app.component("ImageGrid", ImageGrid);
    app.component("Image", Image);
  },
});
