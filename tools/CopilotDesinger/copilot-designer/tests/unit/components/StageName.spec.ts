import { shallowMount } from "@vue/test-utils";
import StageName from "@/components/StageName.vue";

describe("StageName.vue", () => {
  it("should render correct contents", () => {
    const testValue = "test";
    const wrapper = shallowMount(StageName, {
      props: {
        value: testValue,
      },
    });
    expect(wrapper.find("input").element.value).toBe(testValue);
  });
  it("should trigger event on input", async () => {
    const testValue = "test";
    const testValue2 = "test2";
    const wrapper = shallowMount(StageName, {
      props: {
        value: testValue,
      },
    });
    await wrapper.find("input").setValue(testValue2);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const value = (emitted.update[0] as [string])[0];
    expect(value).toBe(testValue2);
  });
});
