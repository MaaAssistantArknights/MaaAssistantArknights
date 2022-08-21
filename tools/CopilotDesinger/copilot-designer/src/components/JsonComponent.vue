<template>
  <button class="btn btn-primary" @click="downloadJson">下载JSON</button>
  <input
    type="file"
    style="display: none"
    name="files[]"
    size="1"
    ref="jsonFile"
    @change="uploadJson"
  />
  <button class="btn btn-primary" @click="beforeUploadJson">载入JSON</button>
</template>

<script lang="ts">
import CopilotData, { createEmptyCopilotData } from "@/interfaces/CopilotData";
import { defineComponent, PropType } from "vue";

export default defineComponent({
  props: {
    data: {
      type: Object as PropType<CopilotData>,
    },
  },
  methods: {
    beforeUploadJson() {
      (this.$refs.jsonFile as HTMLInputElement).click();
    },
    uploadJson(e: Event) {
      const files = (e.target as HTMLInputElement).files;
      if (!files) return;
      const f = files[0];
      const reader = new FileReader();
      reader.onload = () => {
        const newData = JSON.parse(reader.result as string);
        this.$emit("loadData", {
          ...createEmptyCopilotData(),
          ...newData,
        } as CopilotData);
      };
      reader.readAsText(f);
    },
    downloadJson() {
      const result = JSON.stringify(this.data, null, 4);
      const file = new Blob([result], { type: "application/json" });
      const fileName =
        (this.data?.stage_name ?? "") +
        "_" +
        (this.data?.opers?.map((o) => o.name).join("+") ?? "") +
        ".json";
      const url = window.URL.createObjectURL(file);
      const link = document.createElement("a");
      link.href = url;
      link.setAttribute("download", fileName);
      link.click();
    },
  },
  emits: ["loadData"],
});
</script>
