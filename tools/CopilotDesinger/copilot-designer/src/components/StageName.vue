<template>
  <h1>关卡名</h1>
  <span class="tips"
    >小贴士：填完关卡名后开一局，会在目录下生成 map.png 地图坐标图片</span
  >
  <input
    type="text"
    id="stage_name"
    class="form-control"
    placeholder="关卡名，中英文均可"
    :value="stageName"
  />
</template>

<script lang="ts">
import { defineComponent } from "vue";

export default defineComponent({
  name: "StageName",
  components: {},
  props: {
    stageName: {
      type: String,
      required: true,
    },
  },
});
</script>
