import { shallowMount } from "@vue/test-utils";
import MoveUpButton from "@/controls/MoveUpButton.vue";
import MoveDownButton from "@/controls/MoveDownButton.vue";
import DeleteButton from "@/controls/DeleteButton.vue";
import { Action } from "@/interfaces/CopilotData";
import ActionList from "@/components/ActionList.vue";
import ActionTypeSelector from "@/controls/ActionTypeSelector.vue";
import DirectionSelector from "@/controls/DirectionSelector.vue";

describe("ActionList.vue", () => {
  const testData: Action[] = [
    {
      type: "Deploy",
      kills: 0,
      cost_changes: 1,
      name: "Lancet-2",
      location: [2, 3],
      direction: "Left",
      skill_usage: 3,
      pre_delay: 5,
      rear_delay: 6,
      doc: "doc",
      doc_color: "#ffffff",
    },
    {
      name: "Castle-3",
    },
  ];
  it("should render correct contents", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    const tbody = wrapper.find("tbody");
    const trs = tbody.findAll("tr");
    expect(trs.length).toBe(testData.length);
    for (let i = 0; i < trs.length; i++) {
      const tds = trs[i].findAll("td");
      expect(tds[0].findComponent(ActionTypeSelector).vm.value).toEqual(
        testData[i].type
      );
      expect(tds[1].find("input").element.value).toBe(
        (testData[i].kills ?? 0).toString()
      );
      expect(tds[2].find("input").element.value).toBe(
        (testData[i].cost_changes ?? 0).toString()
      );
      expect(tds[3].find("input").element.value).toBe(testData[i].name);
      expect(tds[4].find("input").element.value).toBe(
        (testData[i].location ?? [0, 0])[0].toString()
      );
      expect(tds[5].find("input").element.value).toBe(
        (testData[i].location ?? [0, 0])[1].toString()
      );
      expect(tds[6].findComponent(DirectionSelector).vm.value).toBe(
        testData[i].direction
      );
      expect(tds[7].find("input").element.value).toBe(
        (testData[i].pre_delay ?? 0).toString()
      );
      expect(tds[8].find("input").element.value).toBe(
        (testData[i].rear_delay ?? 0).toString()
      );
      expect(tds[9].find("input").element.value).toBe(testData[i].doc ?? "");
      expect(tds[10].find("input").element.value).toBe(
        testData[i].doc_color ?? "#000000"
      );
      expect(tds[11].findComponent(MoveUpButton).exists()).toBeTruthy();
      expect(tds[11].findComponent(MoveDownButton).exists()).toBeTruthy();
      expect(tds[11].findComponent(DeleteButton).exists()).toBeTruthy();
    }
  });
  it("should process action type changes", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    const newType = "Skill";
    wrapper.vm.changeActionType(newType, 0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newActions = (emitted.update[0] as [Action[]])[0];
    expect(newActions).toBeTruthy();
    expect(newActions[0].type).toBe(newType);
  });
  it("should process action kills changes", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    const newKills = 1;
    wrapper.vm.changeKills(newKills.toString(), 0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newActions = (emitted.update[0] as [Action[]])[0];
    expect(newActions).toBeTruthy();
    expect(newActions[0].kills).toBe(newKills);
  });
  it("should process action cost change changes", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    const newCostChanges = 2;
    wrapper.vm.changeCostChanges(newCostChanges.toString(), 0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newActions = (emitted.update[0] as [Action[]])[0];
    expect(newActions).toBeTruthy();
    expect(newActions[0].cost_changes).toBe(newCostChanges);
  });
  it("should process action name changes", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    const newName = "Castle-3";
    wrapper.vm.changeName(newName, 0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newActions = (emitted.update[0] as [Action[]])[0];
    expect(newActions).toBeTruthy();
    expect(newActions[0].name).toBe(newName);
  });
  it("should process action location X changes", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    const newLocationX = 3;
    wrapper.vm.changeLocationX(newLocationX.toString(), 0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newActions = (emitted.update[0] as [Action[]])[0];
    expect(newActions).toBeTruthy();
    expect((newActions[0].location as [number, number])[0]).toBe(newLocationX);
  });
  it("should process action location Y changes", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    const newLocationY = 4;
    wrapper.vm.changeLocationY(newLocationY.toString(), 0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newActions = (emitted.update[0] as [Action[]])[0];
    expect(newActions).toBeTruthy();
    expect((newActions[0].location as [number, number])[1]).toBe(newLocationY);
  });
  it("should process action direction changes", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    const newDirection = "Right";
    wrapper.vm.changeDirection(newDirection, 0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newActions = (emitted.update[0] as [Action[]])[0];
    expect(newActions).toBeTruthy();
    expect(newActions[0].direction).toBe(newDirection);
  });
  it("should process action pre-delay changes", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    const newPreDelay = 6;
    wrapper.vm.changePreDelay(newPreDelay.toString(), 0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newActions = (emitted.update[0] as [Action[]])[0];
    expect(newActions).toBeTruthy();
    expect(newActions[0].pre_delay).toBe(newPreDelay);
  });
  it("should process action rear-delay changes", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    const newRearDelay = 7;
    wrapper.vm.changeRearDelay(newRearDelay.toString(), 0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newActions = (emitted.update[0] as [Action[]])[0];
    expect(newActions).toBeTruthy();
    expect(newActions[0].rear_delay).toBe(newRearDelay);
  });
  it("should process action doc changes", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    const newDoc = "newDoc";
    wrapper.vm.changeDoc(newDoc, 0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newActions = (emitted.update[0] as [Action[]])[0];
    expect(newActions).toBeTruthy();
    expect(newActions[0].doc).toBe(newDoc);
  });
  it("should process action doc color changes", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    const newDocColor = "#66ccff";
    wrapper.vm.changeDocColor(newDocColor, 0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newActions = (emitted.update[0] as [Action[]])[0];
    expect(newActions).toBeTruthy();
    expect(newActions[0].doc_color).toBe(newDocColor);
  });
  it("should be able to move action up", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    wrapper.vm.moveUp(1);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newActions = (emitted.update[0] as [Action[]])[0];
    expect(newActions).toBeTruthy();
    expect(newActions[0]).toMatchObject(testData[1]);
    expect(newActions[1]).toMatchObject(testData[0]);
  });
  it("should not move action up when it is on the top", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    wrapper.vm.moveUp(0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeFalsy();
  });
  it("should be able to move action down", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    wrapper.vm.moveDown(0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newActions = (emitted.update[0] as [Action[]])[0];
    expect(newActions).toBeTruthy();
    expect(newActions[0]).toMatchObject(testData[1]);
    expect(newActions[1]).toMatchObject(testData[0]);
  });
  it("should not move action down when it is at the bottom", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    wrapper.vm.moveDown(1);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeFalsy();
  });
  it("should be able to delete action", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    wrapper.vm.deleteItem(0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newActions = (emitted.update[0] as [Action[]])[0];
    expect(newActions).toBeTruthy();
    expect(newActions.length).toBe(1);
    expect(newActions[0]).toMatchObject(testData[1]);
  });
  it("should be able to add new action", () => {
    const wrapper = shallowMount(ActionList, {
      props: {
        actions: testData,
      },
    });
    wrapper.vm.newItem();
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newActions = (emitted.update[0] as [Action[]])[0];
    expect(newActions).toBeTruthy();
    expect(newActions.length).toBe(3);
    expect(newActions[0]).toMatchObject(testData[0]);
    expect(newActions[1]).toMatchObject(testData[1]);
    expect(newActions[2]).toBeTruthy();
  });
});
