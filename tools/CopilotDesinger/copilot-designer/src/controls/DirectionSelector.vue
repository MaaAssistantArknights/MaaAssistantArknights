<template>
  <select class="form-control" style="width: auto" v-model="modelValue">
    <option value="None">无</option>
    <option value="Up">上</option>
    <option value="Down">下</option>
    <option value="Left">左</option>
    <option value="Right">右</option>
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
