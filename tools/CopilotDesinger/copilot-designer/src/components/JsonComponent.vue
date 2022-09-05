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
import CopilotData from "@/interfaces/CopilotData";
import { checkCopilotData } from "@/utils/CopilotDataChecker";
import { defineComponent, PropType } from "vue";

export default defineComponent({
  props: {
    data: {
      type: Object as PropType<CopilotData>,
    },
  },
  methods: {
    beforeUploadJson() {
      const jsonFile = this.$refs.jsonFile as HTMLInputElement;
      jsonFile.value = "";
      jsonFile.click();
    },
    uploadJson(e: Event) {
      const files = (e.target as HTMLInputElement).files;
      if (!files) return;
      const f = files[0];
      const reader = new FileReader();
      reader.onload = () => {
        try {
          const newData = JSON.parse(reader.result as string);
          this.$emit("loadData", checkCopilotData(newData));
        } catch (e) {
          if (e instanceof Error) {
            window.alert("读取 JSON 时发生错误：\n" + e.message);
          }
        }
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
