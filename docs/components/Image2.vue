<template>
    <div>
      <img v-for="src of displayImageList" class="image" :src="src" alt="" />
    </div>
  </template>
  
  <script lang="ts">
  import { PropType, defineComponent } from "vue";
  import { withBase } from "vuepress/client";
  
  export default defineComponent({
    props: {
      imageList: Array as PropType<
        Array<{ light: string; dark: string } | string>
      >,
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
  
  <style scoped>
  .image {
    box-sizing: border-box;
    width: 50%;
    padding: 9px;
    border-radius: 16px;
  }

  @media (max-width: 419px) {
    .image {
      width: auto;
    }
  }
  </style>
  