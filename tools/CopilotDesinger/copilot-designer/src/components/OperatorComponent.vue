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
        <td>
          <OperatorName
            :value="operator.name"
            @input="(e) => changeOperatorName(e.target.value, id)"
          />
        </td>
        <td>
          <input
            type="number"
            class="form-control"
            :value="String(operator.skill ?? 0)"
            @input="(e) => changeOperatorSkill((e.target as HTMLInputElement).value, id)"
          />
        </td>
        <td>
          <input
            type="number"
            class="form-control"
            :value="String(operator.skill_usage ?? 0)"
            @input="(e) => changeOperatorSkillUsage((e.target as HTMLInputElement).value, id)"
          />
        </td>
        <td>
          <MoveUpButton @click="() => moveUp(id)" />
          <MoveDownButton @click="() => moveDown(id)" />
          <DeleteButton @click="() => deleteItem(id)" />
        </td>
      </tr>
    </tbody>
  </table>
  <button class="btn btn-primary" @click="newItem">新增</button>
</template>

<script lang="ts">
import { Operator, Skill } from "@/interfaces/CopilotData";
import { defineComponent, PropType } from "vue";
import OperatorName from "../controls/OperatorName.vue";
import MoveUpButton from "@/controls/MoveUpButton.vue";
import MoveDownButton from "@/controls/MoveDownButton.vue";
import DeleteButton from "@/controls/DeleteButton.vue";

export default defineComponent({
  props: {
    operators: {
      type: Array as PropType<Operator[]>,
      required: true,
    },
  },
  components: { OperatorName, MoveUpButton, MoveDownButton, DeleteButton },
  methods: {
    changeOperatorName(value: string, id: number) {
      const newOperators = [...this.operators];
      newOperators[id].name = value;
      this.$emit("update", newOperators);
    },
    changeOperatorSkill(value: string, id: number) {
      let numValue = value !== "" ? Number(value) : undefined;
      if (numValue !== undefined) {
        if (numValue < 0) {
          numValue = 0;
        } else if (numValue > 3) {
          numValue = 3;
        }
      }

      const newOperators = [...this.operators];
      newOperators[id].skill = numValue as Skill | undefined;
      this.$emit("update", newOperators);
    },
    changeOperatorSkillUsage(value: string, id: number) {
      const numValue = value !== "" ? Number(value) : undefined;
      const newOperators = [...this.operators];
      newOperators[id].skill_usage = numValue;
      this.$emit("update", newOperators);
    },
    moveUp(id: number) {
      if (id > 0) {
        const newOperators = [...this.operators];
        [newOperators[id - 1], newOperators[id]] = [
          newOperators[id],
          newOperators[id - 1],
        ];
        this.$emit("update", newOperators);
      }
    },
    moveDown(id: number) {
      if (id < this.operators.length - 1) {
        const newOperators = [...this.operators];
        [newOperators[id + 1], newOperators[id]] = [
          newOperators[id],
          newOperators[id + 1],
        ];
        this.$emit("update", newOperators);
      }
    },
    deleteItem(id: number) {
      const newOperators = [...this.operators];
      newOperators.splice(id, 1);
      this.$emit("update", newOperators);
    },
    newItem() {
      const newOperators = [...this.operators];
      newOperators.push({
        name: "",
      });
      this.$emit("update", newOperators);
    },
  },
  emits: ["update"],
});
</script>
