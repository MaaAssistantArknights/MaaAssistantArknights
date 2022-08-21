<template>
  <select class="form-control" style="width: auto" v-model="modelValue">
    <option value="无"></option>
    <option value="上">上</option>
    <option value="下">下</option>
    <option value="左">左</option>
    <option value="右">右</option>
  </select>
</template>

<script lang="ts">
import { Direction } from "@/interfaces/CopilotData";
import { defineComponent, PropType } from "vue";

export default defineComponent({
  props: {
    value: String as PropType<Direction>,
  },
  computed: {
    modelValue: {
      get() {
        return this.value ?? "";
      },
      set(value: Direction) {
        this.$emit("update", value);
      },
    },
  },
  emits: ["update"],
});
</script>
