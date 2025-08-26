<template>
  <CardGrid>
    <ImageCard 
      v-for="(item, index) of displayImageList" 
      :key="index"
      :image="item" 
    />
  </CardGrid>
</template>

<script lang="ts">
import { PropType, defineComponent, computed, ref, onMounted, onUnmounted } from "vue";
import { withBase } from "vuepress/client";

export default defineComponent({
  name: "ImageCardGrid",
  props: {
    imageList: {
      type: Array as PropType<Array<{ light: string; dark: string } | string>>,
      required: true,
    },
  },
  setup(props) {
    const isDarkMode = ref(false);
    let observer: MutationObserver | null = null;
    let mediaQuery: MediaQueryList | null = null;

    const updateDarkMode = () => {
      if (typeof window !== 'undefined') {
        const html = document.documentElement;
        isDarkMode.value = html.classList.contains('dark') || 
                          html.getAttribute('data-theme') === 'dark' ||
                          window.matchMedia('(prefers-color-scheme: dark)').matches;
      }
    };

    onMounted(() => {
      updateDarkMode();
      
      if (typeof window !== 'undefined') {
        // 监听DOM变化（主题切换通常会改变class或data-theme属性）
        observer = new MutationObserver(updateDarkMode);
        observer.observe(document.documentElement, {
          attributes: true,
          attributeFilter: ['class', 'data-theme']
        });

        // 监听系统主题变化
        mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');
        mediaQuery.addEventListener('change', updateDarkMode);
      }
    });

    onUnmounted(() => {
      if (observer) {
        observer.disconnect();
      }
      if (mediaQuery) {
        mediaQuery.removeEventListener('change', updateDarkMode);
      }
    });
    
    const displayImageList = computed(() => {
      return props.imageList.map((item) => {
        const src =
          typeof item === "string"
            ? item
            : isDarkMode.value
            ? item.dark
            : item.light;
        return withBase(src);
      });
    });

    return {
      displayImageList,
    };
  },
});
</script>
