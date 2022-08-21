import { shallowMount } from "@vue/test-utils";
import OperatorComponent from "@/components/OperatorComponent.vue";
import OperatorName from "@/controls/OperatorName.vue";
import MoveUpButton from "@/controls/MoveUpButton.vue";
import MoveDownButton from "@/controls/MoveDownButton.vue";
import DeleteButton from "@/controls/DeleteButton.vue";
import { Operator } from "@/interfaces/CopilotData";

describe("OperatorComponent.vue", () => {
  const testData: Operator[] = [
    {
      name: "Lancet-2",
      skill: 1,
      skill_usage: 1,
    },
    {
      name: "Castle-3",
      skill: 1,
      skill_usage: 2,
    },
  ];
  it("should render correct contents", () => {
    const wrapper = shallowMount(OperatorComponent, {
      props: {
        operators: testData,
      },
    });
    const tbody = wrapper.find("tbody");
    const trs = tbody.findAll("tr");
    expect(trs.length).toBe(testData.length);
    for (let i = 0; i < trs.length; i++) {
      const tds = trs[i].findAll("td");
      expect(tds[0].findComponent(OperatorName).vm.value).toEqual(
        testData[i].name
      );
      expect(tds[1].find("input").element.value).toBe(
        testData[i].skill?.toString()
      );
      expect(tds[2].find("input").element.value).toBe(
        testData[i].skill_usage?.toString()
      );
      expect(tds[3].findComponent(MoveUpButton).exists()).toBeTruthy();
      expect(tds[3].findComponent(MoveDownButton).exists()).toBeTruthy();
      expect(tds[3].findComponent(DeleteButton).exists()).toBeTruthy();
    }
  });
  it("should process operator name changes", () => {
    const wrapper = shallowMount(OperatorComponent, {
      props: {
        operators: testData,
      },
    });
    const newName = "Castle-3";
    wrapper.vm.changeOperatorName(newName, 0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newOperators = (emitted.update[0] as [Operator[]])[0];
    expect(newOperators).toBeTruthy();
    expect(newOperators[0].name).toBe(newName);
  });
  it("should process operator skill changes", () => {
    const wrapper = shallowMount(OperatorComponent, {
      props: {
        operators: testData,
      },
    });
    const newSkill = 1;
    wrapper.vm.changeOperatorSkill(newSkill, 0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newOperators = (emitted.update[0] as [Operator[]])[0];
    expect(newOperators).toBeTruthy();
    expect(newOperators[0].skill).toBe(newSkill);
  });
  it("should process operator skill usage changes", () => {
    const wrapper = shallowMount(OperatorComponent, {
      props: {
        operators: testData,
      },
    });
    const newSkillUsage = 2;
    wrapper.vm.changeOperatorSkillUsage(newSkillUsage, 0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newOperators = (emitted.update[0] as [Operator[]])[0];
    expect(newOperators).toBeTruthy();
    expect(newOperators[0].skill_usage).toBe(newSkillUsage);
  });
  it("should be able to move operator up", () => {
    const wrapper = shallowMount(OperatorComponent, {
      props: {
        operators: testData,
      },
    });
    wrapper.vm.moveUp(1);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newOperators = (emitted.update[0] as [Operator[]])[0];
    expect(newOperators).toBeTruthy();
    expect(newOperators[0]).toMatchObject(testData[1]);
    expect(newOperators[1]).toMatchObject(testData[0]);
  });
  it("should not move operator up when it is on the top", () => {
    const wrapper = shallowMount(OperatorComponent, {
      props: {
        operators: testData,
      },
    });
    wrapper.vm.moveUp(0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeFalsy();
  });
  it("should be able to move operator down", () => {
    const wrapper = shallowMount(OperatorComponent, {
      props: {
        operators: testData,
      },
    });
    wrapper.vm.moveDown(0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newOperators = (emitted.update[0] as [Operator[]])[0];
    expect(newOperators).toBeTruthy();
    expect(newOperators[0]).toMatchObject(testData[1]);
    expect(newOperators[1]).toMatchObject(testData[0]);
  });
  it("should not move operator down when it is at the bottom", () => {
    const wrapper = shallowMount(OperatorComponent, {
      props: {
        operators: testData,
      },
    });
    wrapper.vm.moveDown(1);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeFalsy();
  });
  it("should be able to delete operator", () => {
    const wrapper = shallowMount(OperatorComponent, {
      props: {
        operators: testData,
      },
    });
    wrapper.vm.deleteItem(0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newOperators = (emitted.update[0] as [Operator[]])[0];
    expect(newOperators).toBeTruthy();
    expect(newOperators.length).toBe(1);
    expect(newOperators[0]).toMatchObject(testData[1]);
  });
  it("should be able to add new operator", () => {
    const wrapper = shallowMount(OperatorComponent, {
      props: {
        operators: testData,
      },
    });
    wrapper.vm.newItem();
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newOperators = (emitted.update[0] as [Operator[]])[0];
    expect(newOperators).toBeTruthy();
    expect(newOperators.length).toBe(3);
    expect(newOperators[0]).toMatchObject(testData[0]);
    expect(newOperators[1]).toMatchObject(testData[1]);
    expect(newOperators[2]).toBeTruthy();
  });
});
