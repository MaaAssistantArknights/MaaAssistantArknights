<template>
  <select class="form-control" style="width: auto" v-model="modelValue">
    <option value="二倍速">二倍速</option>
    <option value="部署">部署</option>
    <option value="技能">技能</option>
    <option value="撤退">撤退</option>
    <option value="子弹时间">子弹时间</option>
    <option value="技能用法">技能用法</option>
  </select>
</template>

<script lang="ts">
import { ActionType } from "@/interfaces/CopilotData";
import { defineComponent, PropType } from "vue";

export default defineComponent({
  props: {
    value: String as PropType<ActionType>,
  },
  computed: {
    modelValue: {
      get() {
        return this.value ?? "";
      },
      set(value: ActionType) {
        this.$emit("update", value);
      },
    },
  },
  emits: ["update"],
});
</script>
