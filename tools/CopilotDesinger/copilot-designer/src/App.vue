<template>
  <MainContainer />
</template>

<script lang="ts">
import { defineComponent } from "vue";
import MainContainer from "./components/MainContainer.vue";

export default defineComponent({
  name: "App",
  components: {
    MainContainer,
  },
});
</script>

<style></style>
