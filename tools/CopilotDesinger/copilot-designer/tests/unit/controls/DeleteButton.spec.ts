import { shallowMount } from "@vue/test-utils";
import DeleteButton from "@/controls/DeleteButton.vue";

describe("DeleteButton.vue", () => {
  it("should trigger event on click", () => {
    const wrapper = shallowMount(DeleteButton, {});
    wrapper.find("button").trigger("click");
    expect(wrapper.emitted().click).toBeTruthy();
  });
});
