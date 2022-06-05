<template>
  <h2>标题</h2>
  <input
    type="text"
    id="title"
    class="form-control"
    placeholder="标题"
    :value="value"
    @input="update"
  />
</template>

<script lang="ts">
import { defineComponent } from "vue";

export default defineComponent({
  props: {
    value: String,
  },
  methods: {
    update(e: Event) {
      this.$emit("update", (e.target as HTMLInputElement).value);
    },
  },
  emits: ["update"],
});
</script>
