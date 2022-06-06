<template>
  <h2>干员</h2>
  <table class="table table-bordered table-condensed table-striped">
    <thead>
      <tr>
        <th>干员名字</th>
        <th>技能</th>
        <th>技能用法</th>
        <th></th>
      </tr>
    </thead>
    <tbody>
      <tr v-for="(operator, id) in operators" :key="id">
        <td><OperatorName :value="operator.name" /></td>
        <td>
          <input
            type="number"
            class="form-control"
            :value="String(operator.skill ?? 0)"
          />
        </td>
        <td>
          <input
            type="number"
            class="form-control"
            :value="String(operator.skill_usage ?? 0)"
          />
        </td>
        <td></td>
      </tr>
    </tbody>
  </table>
  <button id="opers_new" class="btn btn-primary">新增</button>
</template>

<script lang="ts">
import { Operator } from "@/interfaces/CopilotData";
import { defineComponent, PropType } from "vue";
import OperatorName from "../controls/OperatorName.vue";

export default defineComponent({
  props: {
    operators: {
      type: Array as PropType<Operator[]>,
      required: true,
    },
  },
  components: { OperatorName },
});
</script>
