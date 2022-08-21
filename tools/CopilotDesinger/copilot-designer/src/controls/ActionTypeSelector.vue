<template>
  <select class="form-control" style="width: auto" v-model="modelValue">
    <option value="Deploy">部署</option>
    <option value="Skill">技能</option>
    <option value="Retreat">撤退</option>
    <option value="SpeedUp">二倍速</option>
    <option value="BulletTime">子弹时间</option>
    <option value="SkillUsage">技能用法</option>
    <option value="Output">打印</option>
    <option value="SkillDaemon">摆完挂机</option>
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
