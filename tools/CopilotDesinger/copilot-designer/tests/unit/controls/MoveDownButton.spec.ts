import { shallowMount } from "@vue/test-utils";
import MoveDownButton from "@/controls/MoveDownButton.vue";

describe("MoveDownButton.vue", () => {
  it("should trigger event on click", () => {
    const wrapper = shallowMount(MoveDownButton, {});
    wrapper.find("button").trigger("click");
    expect(wrapper.emitted().click).toBeTruthy();
  });
});
