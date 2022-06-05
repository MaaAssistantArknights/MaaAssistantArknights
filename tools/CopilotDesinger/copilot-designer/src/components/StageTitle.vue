<template>
  <h2>标题</h2>
  <input
    type="text"
    id="title"
    class="form-control"
    placeholder="标题"
    :input="modelValue"
  />
</template>

<script lang="ts">
import { defineComponent } from "vue";

export default defineComponent({
  props: {
    value: String,
  },
  computed: {
    modelValue: {
      get() {
        return this.value ?? "";
      },
      set(value: string) {
        this.$emit("update", value);
      },
    },
  },
});
</script>
