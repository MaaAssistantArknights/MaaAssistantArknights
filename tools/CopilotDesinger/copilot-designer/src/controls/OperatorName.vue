<template>
  <input
    type="text"
    class="form-control"
    placeholder="干员名字"
    list="operatorNames"
    :value="value"
    @input="input"
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
    input(e: Event) {
      this.$emit("input", (e.target as HTMLInputElement).value);
    },
  },
  emits: ["input"],
});
</script>
