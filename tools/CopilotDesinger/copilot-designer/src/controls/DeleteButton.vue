<template>
  <button type="button" class="btn btn-danger" @click="click">
    <i class="fa-solid fa-xmark"></i>
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
