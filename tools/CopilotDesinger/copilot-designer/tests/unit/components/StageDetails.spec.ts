import { shallowMount } from "@vue/test-utils";
import StageDetails from "@/components/StageDetails.vue";

describe("StageDetails.vue", () => {
  it("should render correct contents", () => {
    const testValue = "test";
    const wrapper = shallowMount(StageDetails, {
      props: {
        value: testValue,
      },
    });
    expect(wrapper.find("textarea").element.value).toBe(testValue);
  });
  it("should trigger event on input", async () => {
    const testValue = "test";
    const testValue2 = "test2";
    const wrapper = shallowMount(StageDetails, {
      props: {
        value: testValue,
      },
    });
    await wrapper.find("textarea").setValue(testValue2);
    const emitted = wrapper.emitted();
    expect(emitted.update).toBeTruthy();
    const value = (emitted.update[0] as [string])[0];
    expect(value).toBe(testValue2);
  });
});
