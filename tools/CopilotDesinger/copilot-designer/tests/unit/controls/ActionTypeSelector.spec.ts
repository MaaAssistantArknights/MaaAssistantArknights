import { shallowMount } from "@vue/test-utils";
import ActionTypeSelector from "@/controls/ActionTypeSelector.vue";

describe("ActionTypeSelector.vue", () => {
  it("should render correct contents", () => {
    const testValue = "二倍速";
    const wrapper = shallowMount(ActionTypeSelector, {
      props: {
        value: testValue,
      },
    });
    expect(wrapper.find("select").element.value).toBe(testValue);
  });
  it("should trigger event on input", async () => {
    const testValue = "二倍速";
    const testValue2 = "子弹时间";
    const wrapper = shallowMount(ActionTypeSelector, {
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
