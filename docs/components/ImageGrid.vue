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
import { PropType, defineComponent } from "vue";
import { withBase } from "vuepress/client";

export default defineComponent({
  name: "ImageCardGrid",
  props: {
    imageList: {
      type: Array as PropType<Array<{ light: string; dark: string } | string>>,
      required: true,
    },
  },
  computed: {
    displayImageList() {
      return this.imageList.map((item) => {
        const src =
          typeof item === "string"
            ? item
            : this.$isDarkmode
            ? item.dark
            : item.light;
        return withBase(src);
      });
    },
  },
});
</script>
