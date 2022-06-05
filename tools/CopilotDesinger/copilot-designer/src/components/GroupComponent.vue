<template>
  <h2>群组</h2>
  <div v-for="(group, id) in groups" :key="id">
    <div class="row">
      <div class="col-10">
        <input type="text" class="form-control" :value="group.name" />
        <OperatorComponent :operators="group.opers" />
      </div>
    </div>
  </div>
  <button class="btn btn-primary">添加群组</button>
</template>

<script lang="ts">
import { Group } from "@/interfaces/CopilotData";
import { defineComponent, PropType } from "vue";
import OperatorComponent from "./OperatorComponent.vue";

export default defineComponent({
  props: {
    groups: {
      type: Array as PropType<Group[]>,
      required: true,
    },
  },
  components: { OperatorComponent },
});
</script>
