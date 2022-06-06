<template>
  <input
    type="text"
    class="form-control"
    placeholder="干员名字"
    list="oper_names"
    :value="value"
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
});
</script>
