<template>
  <h2>干员</h2>
  <table id="opers" class="table table-bordered table-condensed table-striped">
    <thead>
      <tr>
        <th>干员名字</th>
        <th>技能</th>
        <th>技能用法</th>
        <th></th>
      </tr>
    </thead>
    <tbody></tbody>
  </table>
  <button id="opers_new" class="btn btn-primary">新增</button>
</template>

<script lang="ts">
import { defineComponent } from "vue";

export default defineComponent({
  name: "OperatorComponent",
  components: {},
});
</script>
