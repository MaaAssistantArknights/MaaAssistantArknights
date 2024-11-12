import { defineClientConfig } from "vuepress/client";
import Image1 from "../components/Image1.vue";
import Image2 from "../components/Image2.vue";
import Image4 from "../components/Image4.vue";

export default defineClientConfig({
  enhance: ({ app }) => {
    app.component("Image1", Image1);
    app.component("Image2", Image2);
    app.component("Image4", Image4);
  },
});
