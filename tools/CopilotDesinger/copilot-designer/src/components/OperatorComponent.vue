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
            :key="id"
            @input="changeOperatorName"
          />
        </td>
        <td>
          <input
            type="number"
            class="form-control"
            :value="String(operator.skill ?? 0)"
            :key="id"
            @input="changeOperatorSkill"
          />
        </td>
        <td>
          <input
            type="number"
            class="form-control"
            :key="id"
            :value="String(operator.skill_usage ?? 0)"
          />
        </td>
        <td>
          <MoveUpButton :key="id" @click="moveUp" />
          <MoveDownButton :key="id" @click="moveDown" />
          <DeleteButton :key="id" @click="deleteItem" />
        </td>
      </tr>
    </tbody>
  </table>
  <button id="opers_new" class="btn btn-primary">新增</button>
</template>

<script lang="ts">
import { Operator } from "@/interfaces/CopilotData";
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
    changeOperatorName(e: Event) {
      const input = e.target as HTMLInputElement;
      this.$emit("update:operator-name", this.$.vnode.key, input.value);
    },
    changeOperatorSkill(e: Event) {
      const input = e.target as HTMLInputElement;
      this.$emit("update:operator-skill", this.$.vnode.key, input.value);
    },
    changeOperatorSkillUsage(e: Event) {
      const input = e.target as HTMLInputElement;
      this.$emit("update:operator-skill-usage", this.$.vnode.key, input.value);
    },
    moveUp() {
      this.$emit("move-up", this.$.vnode.key);
    },
    moveDown() {
      this.$emit("move-down", this.$.vnode.key);
    },
    deleteItem() {
      this.$emit("delete-item", this.$.vnode.key);
    },
  },
  emits: [
    "update:operator-name",
    "update:operator-skill",
    "update:operator-skill-usage",
    "move-up",
    "move-down",
    "delete-item",
  ],
});
</script>
