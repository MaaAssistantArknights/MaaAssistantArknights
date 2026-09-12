"""Run the actual navigation methods against a simulated controller (no emulator)."""

from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "build/craft-navigation-check/flow"
SOURCE = ROOT / "src/MaaCore/Task/Infrast/InfrastMaterialCraftTask.cpp"


def extract(source, name):
    start = source.index(f"bool InfrastMaterialCraftTask::{name}()")
    brace = source.index("{", start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end].replace("InfrastMaterialCraftTask::", "Harness::")


FIXTURE = r"""
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
enum Page { Anywhere, Overview, Room, Craft, Selector, OtherRoom, Mastery, Training };
namespace cv { struct Mat { Page page; }; }
namespace utils { using path = std::filesystem::path; }
struct Logger { template<class... T> void warn(T&&...) {} } Log;
struct Config { unsigned post_delay = 1; };
struct Registry { Config value; Config* get(const char*) { return &value; } } Task;
struct Harness;
struct ProcessTask {
    Harness& harness; std::string name;
    ProcessTask(Harness& h, std::initializer_list<const char*> names) : harness(h), name(*names.begin()) {}
    bool run();
};
struct Harness {
    Page page = Anywhere;
    bool stopped = false, cancel_on_enter = false, cancel_on_swipe = false, tab_no_effect = false;
    int required_view = 0, view = 0, wrong_rooms = 0, tab_success_at = 1;
    int begins = 0, entries = 0, tabs = 0, lefts = 0, rights = 0, invalid_entries = 0;
    int backs = 0;
    std::string failing_process;
    bool need_exit() const { return stopped; }
    Harness* ctrler() { return this; }
    cv::Mat get_image() { return {page}; }
    bool is_craft_page(const cv::Mat& image) { return image.page == Craft; }
    bool is_formula_selector(const cv::Mat& image) { return image.page == Selector; }
    bool is_processing_room(const cv::Mat& image) { return image.page == Room; }
    std::optional<int> match_workshop_template(const cv::Mat& image, const char*) {
        return image.page == Room ? std::optional(1) : std::nullopt;
    }
    bool craft_sleep(unsigned) { return !stopped; }
    void save_img(const std::filesystem::path&) {}
    bool enter_facility() {
        ++entries;
        if (page != Overview) { ++invalid_entries; return false; }
        if (view < required_view) return false;
        page = wrong_rooms-- > 0 ? OtherRoom : Room;
        if (cancel_on_enter) stopped = true;
        return true;
    }
    void swipe_to_the_left_of_main_ui() { ++lefts; view = 1; if (cancel_on_swipe) stopped = true; }
    void swipe_to_right_of_main_ui() { ++rights; view = 2; }
    bool ensure_processing_room();
    bool ensure_craft_page();
    bool navigate() { return ensure_processing_room() && ensure_craft_page(); }
};
bool ProcessTask::run() {
    if (harness.stopped) return false;
    if (name == harness.failing_process) return false;
    if (name == "MaterialCraft-LeaveTraining") {
        if(harness.page==Mastery){harness.backs+=2;harness.page=Training;}
        if(harness.page==Training){++harness.backs;harness.page=Overview;}
    } else if (name == "MaterialCraft@InfrastBegin") {
        if(harness.page==Mastery || harness.page==Training)throw std::runtime_error("Training page not left");
        ++harness.begins; harness.page = Overview; harness.view = 0;
    } else if (name == "MaterialCraft-EnterCraftPage") {
        if (harness.page != Room) throw std::runtime_error("Clicked craft tab outside the workshop");
        ++harness.tabs;
        if (!harness.tab_no_effect && harness.tabs >= harness.tab_success_at) harness.page = Craft;
    } else throw std::runtime_error("Unexpected navigation action: " + name);
    return true;
}
// ACTUAL_METHODS
int main() {
    int checks = 0;
    auto require = [&](bool value) { ++checks; if (!value) throw std::runtime_error("Check failed: " + std::to_string(checks)); };
    for (auto page : {Craft, Selector}) {
        Harness h; h.page = page;
        require(h.navigate()); require(h.begins == 0 && h.entries == 0 && h.tabs == 0);
    }
    { Harness h; h.page = Room;
      require(h.navigate()); require(h.begins == 0 && h.entries == 0 && h.tabs == 1); }
    for (auto page : {Anywhere, Overview, OtherRoom}) {
        Harness h; h.page = page;
        require(h.navigate()); require(h.begins == 1 && h.entries == 1 && h.tabs == 1 && h.lefts == 0 && h.rights == 0);
        require(h.backs==0);
    }
    for(auto page:{Mastery,Training}){Harness h;h.page=page;require(h.navigate());require(h.backs==(page==Mastery?3:1));}
    {Harness h;h.page=Mastery;h.failing_process="MaterialCraft-LeaveTraining";require(!h.navigate());require(h.entries==0);}
    { Harness h; h.required_view = 1;
      require(h.navigate()); require(h.entries == 2 && h.lefts == 1 && h.rights == 0); }
    { Harness h; h.required_view = 2;
      require(h.navigate()); require(h.entries == 3 && h.lefts == 1 && h.rights == 1); }
    { Harness h; h.wrong_rooms = 1;
      require(h.navigate()); require(h.begins == 2 && h.tabs == 1 && h.invalid_entries == 0); }
    { Harness h; h.wrong_rooms = 2;
      require(!h.navigate()); require(h.begins == 2 && h.tabs == 0 && h.invalid_entries == 0); }
    { Harness h; h.required_view = 3;
      require(!h.navigate()); require(h.begins == 2 && h.entries == 6 && h.tabs == 0); }
    { Harness h; h.stopped = true;
      require(!h.navigate()); require(h.begins == 0 && h.entries == 0 && h.tabs == 0); }
    { Harness h; h.cancel_on_enter = true;
      require(!h.navigate()); require(h.entries == 1 && h.tabs == 0); }
    { Harness h; h.required_view = 2; h.cancel_on_swipe = true;
      require(!h.navigate()); require(h.entries == 1 && h.lefts == 1 && h.rights == 0 && h.tabs == 0); }
    { Harness h; h.failing_process = "MaterialCraft@InfrastBegin";
      require(!h.navigate()); require(h.entries == 0 && h.tabs == 0); }
    { Harness h; h.failing_process = "MaterialCraft-EnterCraftPage";
      require(!h.navigate()); require(h.entries == 1 && h.tabs == 0); }
    { Harness h; h.page = Room; h.tab_no_effect = true;
      require(!h.navigate()); require(h.tabs == 3); }
    { Harness h; h.page = Room; h.tab_success_at = 3;
      require(h.navigate()); require(h.tabs == 3); }
    std::cout << "MaterialCraft navigation: " << checks << " checks passed\n";
}
"""


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    source = SOURCE.read_text(encoding="utf-8")
    methods = "\n".join(
        extract(source, name)
        for name in ("ensure_processing_room", "ensure_craft_page")
    )
    (OUT / "main.cpp").write_text(
        FIXTURE.replace("// ACTUAL_METHODS", methods), encoding="utf-8"
    )
    (OUT / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.20)\nproject(CraftNavigation LANGUAGES CXX)\n"
        "add_executable(navigation main.cpp)\ntarget_compile_features(navigation PRIVATE cxx_std_20)\n",
        encoding="utf-8",
    )
    subprocess.run(["cmake", "-S", str(OUT), "-B", str(OUT / "native")], check=True)
    subprocess.run(
        ["cmake", "--build", str(OUT / "native"), "--config", "Debug"], check=True
    )
    candidates = [OUT / "native/Debug/navigation.exe", OUT / "native/navigation"]
    subprocess.run(
        [str(next(path for path in candidates if path.exists()))], check=True
    )


if __name__ == "__main__":
    main()
