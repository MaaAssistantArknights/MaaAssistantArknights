<template>
  <h2>描述</h2>
  <textarea
    class="form-control"
    placeholder="作业作者、参考的视频、危机合约要选哪些tag等等"
    v-model="modelValue"
  ></textarea>
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
  computed: {
    modelValue: {
      get() {
        return this.value;
      },
      set(value: string) {
        this.$emit("update", value);
      },
    },
  },
  emits: ["update"],
});
</script>
