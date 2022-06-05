<template>
  <button id="download_json" class="btn btn-primary">下载JSON</button>
  <input
    type="file"
    id="json_file"
    style="display: none"
    name="files[]"
    size="1"
  />
  <button id="upload_json" class="btn btn-primary">载入JSON</button>
</template>

<script lang="ts">
import { defineComponent } from "vue";

export default defineComponent({
  name: "JsonComponent",
  components: {},
});
</script>
