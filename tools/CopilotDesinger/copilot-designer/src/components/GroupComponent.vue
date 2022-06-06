<template>
  <h2>群组</h2>
  <div v-for="(group, id) in groups" :key="id">
    <div class="row">
      <div class="col-10">
        <input
          type="text"
          class="form-control"
          :value="group.name"
          @input="(e) => updateName((e.target as HTMLInputElement).value, id)"
        />
        <OperatorComponent
          :operators="group.opers"
          @update="(newOperators) => updateOperators(newOperators, id)"
        />
      </div>
      <div class="col-2">
        <MoveUpButton />
        <MoveDownButton />
        <DeleteButton />
      </div>
    </div>
  </div>
  <button class="btn btn-primary" @click="newItem">添加群组</button>
</template>

<script lang="ts">
import { Group, Operator } from "@/interfaces/CopilotData";
import { defineComponent, PropType } from "vue";
import OperatorComponent from "./OperatorComponent.vue";
import MoveUpButton from "@/controls/MoveUpButton.vue";
import MoveDownButton from "@/controls/MoveDownButton.vue";
import DeleteButton from "@/controls/DeleteButton.vue";

export default defineComponent({
  props: {
    groups: {
      type: Array as PropType<Group[]>,
      required: true,
    },
  },
  components: { OperatorComponent, MoveUpButton, MoveDownButton, DeleteButton },
  methods: {
    updateName(value: string, id: number) {
      const newGroups = [...this.groups];
      newGroups[id].name = value;
      this.$emit("update", newGroups);
    },
    updateOperators(operators: Operator[], id: number) {
      const newGroups = [...this.groups];
      newGroups[id].opers = operators;
      this.$emit("update", newGroups);
    },
    newItem() {
      const newGroups = [...this.groups];
      newGroups.push({
        name: "",
        opers: [],
      });
      this.$emit("update", newGroups);
    },
  },
  emits: ["update"],
});
</script>
