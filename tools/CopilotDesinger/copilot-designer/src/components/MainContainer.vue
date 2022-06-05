<template>
  <div class="container-fluid">
    <div class="row">
      <div class="col-12">
        <JsonComponent />
      </div>
    </div>
    <div class="row">
      <div class="col-12">
        <StageName />
      </div>
      <div class="col-12">
        <StageTitle />
      </div>
      <div class="col-12">
        <StageDetails />
      </div>
      <div class="col-12">
        <GroupComponent />
      </div>
      <div class="col-12">
        <OperatorComponent />
      </div>
      <div class="col-12">
        <BattleProgress />
      </div>
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import JsonComponent from "./JsonComponent.vue";
import StageName from "./StageName.vue";
import StageTitle from "./StageTitle.vue";
import StageDetails from "./StageDetails.vue";
import GroupComponent from "./GroupComponent.vue";
import OperatorComponent from "./OperatorComponent.vue";
import BattleProgress from "./BattleProgress.vue";

export default defineComponent({
  name: "MainContainer",
  components: {
    JsonComponent,
    StageName,
    StageTitle,
    StageDetails,
    GroupComponent,
    OperatorComponent,
    BattleProgress,
  },
});
</script>
