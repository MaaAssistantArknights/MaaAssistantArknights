<template>
  <h1>关卡名</h1>
  <span class="tips"
    >小贴士：填完关卡名后开一局，会在目录下生成 map.png 地图坐标图片</span
  >
  <input
    type="text"
    class="form-control"
    placeholder="关卡名，中英文均可"
    :value="value"
    @input="update"
  />
</template>

<script lang="ts">
import { defineComponent } from "vue";

export default defineComponent({
  props: {
    value: {
      type: String,
      required: true,
    },
  },
  methods: {
    update(e: Event) {
      this.$emit("update", (e.target as HTMLInputElement).value);
    },
  },
  emits: ["update"],
});
</script>
