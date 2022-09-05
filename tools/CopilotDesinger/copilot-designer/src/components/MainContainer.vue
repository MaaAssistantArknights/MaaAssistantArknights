<template>
  <div class="container-fluid">
    <div class="row">
      <div class="col-12">
        <JsonComponent :data="data" @loadData="loadData" />
      </div>
    </div>
    <div class="row">
      <div class="col-12">
        <StageName :value="data.stage_name" @update="updateStageName" />
      </div>
      <div class="col-12">
        <StageTitle :value="data.doc?.title ?? ''" @update="updateStageTitle" />
      </div>
      <div class="col-12">
        <StageDetails
          :value="data.doc?.details ?? ''"
          @update="updateStageDetails"
        />
      </div>
      <div class="col-12">
        <GroupComponent :groups="data.groups ?? []" @update="updateGroups" />
      </div>
      <div class="col-12">
        <OperatorComponent
          :operators="data.opers ?? []"
          @update="updateOperators"
        />
      </div>
      <div class="col-12">
        <ActionList :actions="data.actions" @update="updateActions" />
      </div>
    </div>
  </div>
  <datalist id="operatorNames">
    <option v-for="operatorName in characterList" :key="operatorName">
      {{ operatorName }}
    </option>
  </datalist>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import JsonComponent from "./JsonComponent.vue";
import StageName from "./StageName.vue";
import StageTitle from "./StageTitle.vue";
import StageDetails from "./StageDetails.vue";
import GroupComponent from "./GroupComponent.vue";
import OperatorComponent from "./OperatorComponent.vue";
import ActionList from "./ActionList.vue";
import CopilotData, {
  Action,
  createEmptyCopilotData,
  Group,
  Operator,
} from "@/interfaces/CopilotData";
import { CharacterList } from "@/misc/Characters";

export default defineComponent({
  components: {
    JsonComponent,
    StageName,
    StageTitle,
    StageDetails,
    GroupComponent,
    OperatorComponent,
    ActionList,
  },
  data() {
    return {
      data: createEmptyCopilotData(),
      characterList: CharacterList.sort(),
    };
  },
  methods: {
    loadData(data: CopilotData) {
      this.data = data;
    },
    updateStageName(v: string) {
      this.data.stage_name = v;
    },
    updateStageTitle(v: string) {
      this.data.doc = {
        ...this.data.doc,
        title: v,
      };
    },
    updateStageDetails(v: string) {
      this.data.doc = {
        // `title` is required
        // will be replaced by spread operator below if it exists
        title: "",
        ...this.data.doc,
        details: v,
      };
    },
    updateGroups(groups: Group[]) {
      this.data.groups = groups;
    },
    updateOperators(operators: Operator[]) {
      this.data.opers = operators;
    },
    updateActions(actions: Action[]) {
      this.data.actions = actions;
    },
  },
});
</script>
