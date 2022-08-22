import { shallowMount } from "@vue/test-utils";
import GroupComponent from "@/components/GroupComponent.vue";
import MoveUpButton from "@/controls/MoveUpButton.vue";
import MoveDownButton from "@/controls/MoveDownButton.vue";
import DeleteButton from "@/controls/DeleteButton.vue";
import { Group, Operator } from "@/interfaces/CopilotData";

describe("GroupComponent.vue", () => {
  const testData: Group[] = [
    {
      name: "group 1",
      opers: [] as Operator[],
    },
    {
      name: "group 2",
      opers: [] as Operator[],
    },
  ];
  it("should render correct contents", () => {
    const wrapper = shallowMount(GroupComponent, {
      props: {
        groups: testData,
      },
    });
    const outerDivs = wrapper.findAll("div.group-item");
    expect(outerDivs.length).toBe(testData.length);
    for (let i = 0; i < outerDivs.length; i++) {
      expect(outerDivs[i].find("div.col-10").find("input").element.value).toBe(
        testData[i].name
      );
      expect(
        outerDivs[i].find("div.col-2").findComponent(MoveUpButton).exists()
      ).toBeTruthy();
      expect(
        outerDivs[i].find("div.col-2").findComponent(MoveDownButton).exists()
      ).toBeTruthy();
      expect(
        outerDivs[i].find("div.col-2").findComponent(DeleteButton).exists()
      ).toBeTruthy();
    }
  });
  it("should process group name changes", () => {
    const wrapper = shallowMount(GroupComponent, {
      props: {
        groups: testData,
      },
    });
    const newName = "group 1 new";
    wrapper.vm.updateName(newName, 0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newGroups = (emitted.update[0] as [Group[]])[0];
    expect(newGroups).toBeTruthy();
    expect(newGroups[0].name).toBe(newName);
  });
  it("should be able to move group up", () => {
    const wrapper = shallowMount(GroupComponent, {
      props: {
        groups: testData,
      },
    });
    wrapper.vm.moveUp(1);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newGroups = (emitted.update[0] as [Group[]])[0];
    expect(newGroups).toBeTruthy();
    expect(newGroups[0]).toMatchObject(testData[1]);
    expect(newGroups[1]).toMatchObject(testData[0]);
  });
  it("should not move operator up when it is on the top", () => {
    const wrapper = shallowMount(GroupComponent, {
      props: {
        groups: testData,
      },
    });
    wrapper.vm.moveUp(0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeFalsy();
  });
  it("should be able to move group down", () => {
    const wrapper = shallowMount(GroupComponent, {
      props: {
        groups: testData,
      },
    });
    wrapper.vm.moveDown(0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newGroups = (emitted.update[0] as [Group[]])[0];
    expect(newGroups).toBeTruthy();
    expect(newGroups[0]).toMatchObject(testData[1]);
    expect(newGroups[1]).toMatchObject(testData[0]);
  });
  it("should not move group down when it is at the bottom", () => {
    const wrapper = shallowMount(GroupComponent, {
      props: {
        groups: testData,
      },
    });
    wrapper.vm.moveDown(1);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeFalsy();
  });
  it("should be able to delete group", () => {
    const wrapper = shallowMount(GroupComponent, {
      props: {
        groups: testData,
      },
    });
    wrapper.vm.deleteItem(0);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newGroups = (emitted.update[0] as [Group[]])[0];
    expect(newGroups).toBeTruthy();
    expect(newGroups.length).toBe(1);
    expect(newGroups[0]).toMatchObject(testData[1]);
  });
  it("should be able to add new group", () => {
    const wrapper = shallowMount(GroupComponent, {
      props: {
        groups: testData,
      },
    });
    wrapper.vm.newItem();
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const newGroups = (emitted.update[0] as [Group[]])[0];
    expect(newGroups).toBeTruthy();
    expect(newGroups.length).toBe(3);
    expect(newGroups[0]).toMatchObject(testData[0]);
    expect(newGroups[1]).toMatchObject(testData[1]);
    expect(newGroups[2]).toBeTruthy();
  });
});
