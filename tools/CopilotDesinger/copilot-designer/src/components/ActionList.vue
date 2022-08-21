<template>
  <h2>战斗流程</h2>
  <table class="table table-bordered table-condensed table-striped">
    <thead>
      <tr>
        <th>类别</th>
        <th>击杀数</th>
        <th>费用变化</th>
        <th>干员名字</th>
        <th>x坐标</th>
        <th>y坐标</th>
        <th>方向</th>
        <th>前延迟</th>
        <th>后延迟</th>
        <th>文本</th>
        <th>文本颜色</th>
        <th></th>
      </tr>
    </thead>
    <tbody>
      <tr v-for="(action, id) in actions" :key="id">
        <td>
          <ActionTypeSelector
            :value="action.type"
            @update="(value) => changeActionType(value, id)"
          />
        </td>
        <td>
          <input
            type="number"
            class="form-control"
            :value="String(action.kills ?? 0)"
            @input="(e) => changeKills((e.target as HTMLInputElement).value, id)"
          />
        </td>
        <td>
          <input
            type="number"
            class="form-control"
            :value="String(action.cost_changes ?? 0)"
            @input="(e) => changeCostChanges((e.target as HTMLInputElement).value, id)"
          />
        </td>
        <td>
          <input
            type="text"
            class="form-control"
            :value="action.name"
            @input="(e) => changeName((e.target as HTMLInputElement).value, id)"
          />
        </td>
        <td>
          <input
            type="number"
            class="form-control"
            :value="String((action.location ?? [0, 0])[0] ?? 0)"
            @input="(e) => changeLocationX((e.target as HTMLInputElement).value, id)"
          />
        </td>
        <td>
          <input
            type="number"
            class="form-control"
            :value="String((action.location ?? [0, 0])[1] ?? 0)"
            @input="(e) => changeLocationY((e.target as HTMLInputElement).value, id)"
          />
        </td>
        <td>
          <DirectionSelector
            :value="action.direction"
            @update="(value) => changeDirection(value, id)"
          />
        </td>
        <td>
          <input
            type="number"
            class="form-control"
            :value="String(action.pre_delay ?? 0)"
            @input="(e) => changePreDelay((e.target as HTMLInputElement).value, id)"
          />
        </td>
        <td>
          <input
            type="number"
            class="form-control"
            :value="String(action.rear_delay ?? 0)"
            @input="(e) => changeRearDelay((e.target as HTMLInputElement).value, id)"
          />
        </td>
        <td>
          <input
            type="text"
            class="form-control"
            :value="action.doc"
            @input="(e) => changeDoc((e.target as HTMLInputElement).value, id)"
          />
        </td>
        <td>
          <input
            type="color"
            class="form-control"
            :value="action.doc_color ?? '#000000'"
            @input="(e) => changeDocColor((e.target as HTMLInputElement).value, id)"
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
import { Action, ActionType, Direction } from "@/interfaces/CopilotData";
import { defineComponent, PropType } from "vue";
import ActionTypeSelector from "@/controls/ActionTypeSelector.vue";
import DirectionSelector from "@/controls/DirectionSelector.vue";
import MoveUpButton from "@/controls/MoveUpButton.vue";
import MoveDownButton from "@/controls/MoveDownButton.vue";
import DeleteButton from "@/controls/DeleteButton.vue";

export default defineComponent({
  props: {
    actions: {
      type: Object as PropType<Action[]>,
      required: true,
    },
  },
  components: {
    ActionTypeSelector,
    DirectionSelector,
    MoveUpButton,
    MoveDownButton,
    DeleteButton,
  },
  methods: {
    changeActionType(value: ActionType, id: number) {
      const newActions = [...this.actions];
      newActions[id].type = value as ActionType;
      this.$emit("update", newActions);
    },
    changeKills(value: string, id: number) {
      const newActions = [...this.actions];
      newActions[id].kills = value !== "" ? Number(value) : undefined;
      this.$emit("update", newActions);
    },
    changeCostChanges(value: string, id: number) {
      const newActions = [...this.actions];
      newActions[id].cost_changes = value !== "" ? Number(value) : undefined;
      this.$emit("update", newActions);
    },
    changeName(value: string, id: number) {
      const newActions = [...this.actions];
      newActions[id].name = value;
      this.$emit("update", newActions);
    },
    changeLocationX(value: string, id: number) {
      const newActions = [...this.actions];
      const newLocation = newActions[id].location ?? [0, 0];
      newLocation[0] = Number(value);
      newActions[id].location = newLocation;
      this.$emit("update", newActions);
    },
    changeLocationY(value: string, id: number) {
      const newActions = [...this.actions];
      const newLocation = newActions[id].location ?? [0, 0];
      newLocation[1] = Number(value);
      newActions[id].location = newLocation;
      this.$emit("update", newActions);
    },
    changeDirection(value: Direction, id: number) {
      const newActions = [...this.actions];
      newActions[id].direction = value;
      this.$emit("update", newActions);
    },
    changePreDelay(value: string, id: number) {
      const newActions = [...this.actions];
      newActions[id].pre_delay = value !== "" ? Number(value) : undefined;
      this.$emit("update", newActions);
    },
    changeRearDelay(value: string, id: number) {
      const newActions = [...this.actions];
      newActions[id].rear_delay = value !== "" ? Number(value) : undefined;
      this.$emit("update", newActions);
    },
    changeDoc(value: string, id: number) {
      const newActions = [...this.actions];
      newActions[id].doc = value;
      this.$emit("update", newActions);
    },
    changeDocColor(value: string, id: number) {
      const newActions = [...this.actions];
      newActions[id].doc_color = value;
      this.$emit("update", newActions);
    },
    moveUp(id: number) {
      if (id > 0) {
        const newActions = [...this.actions];
        [newActions[id - 1], newActions[id]] = [
          newActions[id],
          newActions[id - 1],
        ];
        this.$emit("update", newActions);
      }
    },
    moveDown(id: number) {
      if (id < this.actions.length - 1) {
        const newActions = [...this.actions];
        [newActions[id + 1], newActions[id]] = [
          newActions[id],
          newActions[id + 1],
        ];
        this.$emit("update", newActions);
      }
    },
    deleteItem(id: number) {
      const newActions = [...this.actions];
      newActions.splice(id, 1);
      this.$emit("update", newActions);
    },
    newItem() {
      const newActions = [...this.actions];
      newActions.push({
        name: "",
      });
      this.$emit("update", newActions);
    },
  },
  emits: ["update"],
});
</script>
