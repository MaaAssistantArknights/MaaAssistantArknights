import { shallowMount } from "@vue/test-utils";
import DirectionSelector from "@/controls/DirectionSelector.vue";

describe("DirectionSelector.vue", () => {
  it("should render correct contents", () => {
    const testValue = "上";
    const wrapper = shallowMount(DirectionSelector, {
      props: {
        value: testValue,
      },
    });
    expect(wrapper.find("select").element.value).toBe(testValue);
  });
  it("should trigger event on input", async () => {
    const testValue = "上";
    const testValue2 = "下";
    const wrapper = shallowMount(DirectionSelector, {
      props: {
        value: testValue,
      },
    });
    await wrapper.find("select").setValue(testValue2);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    expect((emitted.update[0] as [string])[0]).toBe(testValue2);
  });
});
