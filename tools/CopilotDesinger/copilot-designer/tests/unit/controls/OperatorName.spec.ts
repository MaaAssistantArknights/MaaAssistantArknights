import { shallowMount } from "@vue/test-utils";
import OperatorName from "@/controls/OperatorName.vue";

describe("OperatorName.vue", () => {
  it("should render correct contents", () => {
    const testValue = "Lancet-2";
    const wrapper = shallowMount(OperatorName, {
      props: {
        value: testValue,
      },
    });
    expect(wrapper.find("input").element.value).toBe(testValue);
  });
  it("should trigger event on input", async () => {
    const testValue = "Lancet-2";
    const testValue2 = "Castle-3";
    const wrapper = shallowMount(OperatorName, {
      props: {
        value: testValue,
      },
    });
    await wrapper.find("input").setValue(testValue2);
    const emitted = wrapper.emitted();
    expect(emitted.input).toBeTruthy();
    const event = (emitted.input[0] as [Event])[0] as Event;
    expect((event.target as HTMLInputElement).value).toBe(testValue2);
  });
});
