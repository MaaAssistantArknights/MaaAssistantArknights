import type { VNode } from "vue";
import { defineComponent, h, computed } from "vue";

import NavbarDropdown from "@theme-hope/modules/navbar/components/NavbarDropdown";
import { I18nIcon } from "@theme-hope/modules/navbar/components/icons/index";
import { useNavbarLanguageDropdown } from "@theme-hope/modules/navbar/composables/index";

export default defineComponent({
  name: "LanguageDropdown",

  setup() {
    const dropdown = useNavbarLanguageDropdown();

    const filteredDropdown = computed(() => {
      if (!dropdown.value) return null;

      const filteredConfig = { ...dropdown.value };

      if (filteredConfig.children) {
        filteredConfig.children = filteredConfig.children.filter(item => {
          // 过滤掉跳转到根目录的项目
          return item.link !== '/' && item.link !== '/docs/' && !item.link?.endsWith('/docs/');
        });
      }

      return filteredConfig;
    });

    //console.log("OldLanguageDropdown", dropdown.value);
    //console.log("NewLanguageDropdown", filteredDropdown());

    return (): VNode | null =>
      dropdown.value
        ? h(
          "div",
          { class: "vp-nav-item" },
          h(
            NavbarDropdown,
            { config: filteredDropdown.value },
            {
              title: () =>
                h(I18nIcon, {
                  "aria-label": filteredDropdown.value?.ariaLabel,
                  style: {
                    width: "1rem",
                    height: "1rem",
                    verticalAlign: "middle",
                  },
                }),
            },
          ),
        )
        : null;
  },
});