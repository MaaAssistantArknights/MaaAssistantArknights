import { shallowMount } from "@vue/test-utils";
import MoveUpButton from "@/controls/MoveUpButton.vue";

describe("MoveUpButton.vue", () => {
  it("should trigger event on click", () => {
    const wrapper = shallowMount(MoveUpButton, {});
    wrapper.find("button").trigger("click");
    expect(wrapper.emitted().click).toBeTruthy();
  });
});
