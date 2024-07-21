import { defineClientConfig } from "vuepress/client";
import ImageGrid from "../components/ImageGrid.vue";

export default defineClientConfig({
    enhance: ({ app, router }) => {
        app.component("ImageGrid", ImageGrid);
        // 路由处理,处理搜索结果返回的多出的/docs
        if (router) {
            router.beforeEach((to, from, next) => {
                if (to.fullPath.startsWith("/docs")) {
                    next(to.fullPath.replace("/docs", ""));
                } else {
                    next();
                }
            });
        } else {
            console.error("Router is not available.");
        }
    },
});
