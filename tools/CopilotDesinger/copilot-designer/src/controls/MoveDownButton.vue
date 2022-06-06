<template>
  <button type="button" class="btn btn-primary" @click="click">
    <i class="fa-solid fa-arrow-down"></i>
  </button>
</template>

<script lang="ts">
import { defineComponent } from "vue";

export default defineComponent({
  methods: {
    click() {
      this.$emit("click");
    },
  },
  emits: ["click"],
});
</script>
