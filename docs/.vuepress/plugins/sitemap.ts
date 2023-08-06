import { sitemapPlugin } from "vuepress-plugin-sitemap2";

export default function SitemapPlugin() {
  return sitemapPlugin({
    hostname: "https://maa.plus",
  });
}
