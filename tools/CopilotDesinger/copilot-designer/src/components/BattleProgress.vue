<template>
  <h2>战斗流程</h2>
  <table
    id="actions"
    class="table table-bordered table-condensed table-striped"
  >
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
    <tbody></tbody>
  </table>
  <button id="actions_new" class="btn btn-primary">新增</button>
</template>

<script lang="ts">
import { defineComponent } from "vue";

export default defineComponent({
  name: "BattleProgress",
  components: {},
});
</script>
