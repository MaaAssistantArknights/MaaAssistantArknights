"""Exercise actual factory selection, recipe navigation and restoration methods.

The controller/vision substitutes never connect to a game. Generated C++ and
recovery records stay under build/craft-manufacturing-check.
"""

from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "build/craft-manufacturing-check"
SOURCE = ROOT / "src/MaaCore/Task/Infrast/InfrastMaterialCraftTask_Mfg.cpp"


def extract(source, name):
    start = source.index(f"bool InfrastMaterialCraftTask::{name}(")
    brace = source.index("{", start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end].replace("InfrastMaterialCraftTask::", "Harness::")


FIXTURE = r"""
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>
#include <meojson/json.hpp>
namespace utils { using path = std::filesystem::path; }
struct { std::filesystem::path get() { return std::filesystem::current_path(); } } UserDir;
struct ManufacturingRecipe { std::string item_id, ingredient_id; int batches = 0, weight = 1; };
struct MaterialFormula { std::string item_id, facility; bool is_manufacturing() const { return facility == "Mfg"; } };
struct CraftOperation { MaterialFormula formula; int batches; };
struct Room { ManufacturingRecipe recipe; bool level_three = true, readable = true; int storage = 0; };
enum Page { Other, Overview, Factory, Selector };
struct Harness;
namespace cv { struct Mat { Harness* owner; }; }
struct Rect {
    std::string item;
    int x = 0, y = 0;
    Rect move(std::initializer_list<int>) const { return *this; }
};
struct Candidate { Rect product_rect, click_rect; double score = 1; };
struct InfrastFacilityImageAnalyzer {
    Harness& h;
    InfrastFacilityImageAnalyzer(cv::Mat);
    void set_to_be_analyzed(std::initializer_list<const char*>) {}
    bool analyze();
    size_t get_quantity(const char*);
    Rect get_rect(const char*, int i);
};
struct InfrastMaterialCraftImageAnalyzer {
    Harness& h; std::string item; std::vector<Candidate> candidates;
    InfrastMaterialCraftImageAnalyzer(cv::Mat);
    void set_task_info(const char*) {}
    void set_item_id(const std::string& id) { item = id; }
    template<class F> void set_cancel_check(F) {}
    bool analyze();
    bool analyze_with_name(const char*, const std::string&, double) { return analyze(); }
    auto get_result() { return candidates; }
};
struct MaterialImageAnalyzer {
    MaterialImageAnalyzer(cv::Mat) {}
    void set_item_id(const std::string&) {}
    void set_task_info(const char*) {}
    void set_scales(std::initializer_list<double>) {}
    void set_roi(Rect) {}
    bool analyze() { return true; }
};
struct { std::string get_item_name(const std::string& id) { return id; } } ItemData;
struct ProcessTask {
    Harness& h; std::string name;
    ProcessTask(Harness& owner, std::initializer_list<const char*> names) : h(owner), name(*names.begin()) {}
    bool run();
};
struct Harness {
    std::vector<Room> rooms;
    Page page = Other;
    int m_cur_facility_index = 0, m_next_operation_id = 0;
    bool m_inventory_complete = false, m_replenish_originium_shards = false;
    bool stopped = false, fail_selection = false, fail_collect = false;
    bool missing_formula = false, verify_product = true, bottom = false;
    int fail_entry = -1, cancel_entry = -1, maximum = 99;
    int chips = 0, collections = 0;
    int pans_to_left = 0, overview_swipes = 0;
    std::vector<int> visits;
    std::vector<std::string> actions;
    std::optional<ManufacturingRecipe> pending;
    std::string failure;
    std::string m_facility;
    struct { std::vector<CraftOperation> operations; } m_plan;
    int processing_entries = 0, processing_operations = 0;
    bool build_plan() { return true; }
    bool ensure_processing_room() { ++processing_entries; return !stopped; }
    bool ensure_craft_page() { return !stopped; }
    bool execute_operation(const CraftOperation&) { ++processing_operations; return !stopped; }
    bool _run();
    ManufacturingRecipe recovery_recipe;
    json::value saved_record;
    Room& room() { return rooms.at(m_cur_facility_index); }
    bool need_exit() const { return stopped; }
    Harness* ctrler() { return this; }
    cv::Mat get_image() { return {this}; }
    bool click(const Rect& rect) {
        if (page != Selector) throw std::runtime_error("Click outside selector");
        pending = ManufacturingRecipe {rect.item, rect.item == "3141" ? room().recipe.ingredient_id : "", 0,
                                       rect.item.starts_with("32") ? 5 : room().recipe.weight};
        page = Factory; return true;
    }
    bool is_manufacturing_page(cv::Mat) { return page == Factory; }
    std::optional<int> match_workshop_template(cv::Mat, const std::string& name) {
        if (name == "MaterialCraft-MfgLevel3") return room().level_three ? std::optional(1) : std::nullopt;
        if (name == "MaterialCraft-MfgRoom") return page == Factory ? std::optional(1) : std::nullopt;
        if (name == "MaterialCraft-MfgPending") return pending ? std::optional(1) : std::nullopt;
        if (name == "MaterialCraft-MfgChipsCategory") return page == Selector ? std::optional(1) : std::nullopt;
        throw std::runtime_error("Unexpected template: " + name);
    }
    bool leave_manufacturing_page() { page = Other; return !stopped; }
    void swipe_to_the_left_of_main_ui() { ++overview_swipes; if (pans_to_left > 0) --pans_to_left; }
    bool enter_manufacturing_facility(int index) {
        if (page != Overview) throw std::runtime_error("Enter outside overview");
        visits.push_back(index);
        if (index == fail_entry) return false;
        m_cur_facility_index = index; page = Factory;
        if (index == cancel_entry) stopped = true;
        return !stopped;
    }
    bool click_bottom_left_tab() { return !stopped; }
    std::optional<ManufacturingRecipe> read_manufacturing_recipe() {
        if (stopped || page != Factory || pending || !room().readable || room().recipe.item_id.starts_with("32"))
            return std::nullopt;
        return room().recipe;
    }
    std::optional<std::pair<int, int>> read_manufacturing_number(const std::string& name, bool fraction = false) {
        if (stopped || page != Factory) return std::nullopt;
        if (fraction) return std::pair {room().storage, 50};
        return std::pair {pending ? pending->batches : room().recipe.batches, 0};
    }
    bool manufacturing_product_matches(const std::string& id, cv::Mat) {
        return verify_product && page == Factory && (pending ? pending->item_id : room().recipe.item_id) == id;
    }
    bool craft_sleep(unsigned) { return !stopped; }
    void save_img(const std::filesystem::path&) {}
    bool manufacturing_action(const std::string& name) {
        actions.push_back(name);
        if (stopped) return false;
        if (name == "MaterialCraft-MfgOpenSelector") {
            if (fail_selection) return false;
            page = Selector;
        } else if (name == "MaterialCraft-MfgRewind") bottom = false;
        else if (name == "MaterialCraft-MfgSwipe") bottom = true;
        else if (name == "MaterialCraft-MfgBack") page = Factory;
        else if (name == "MaterialCraft-MfgCancel") pending.reset();
        else if (name.ends_with("Category")) {}
        else {
            if (page != Factory) throw std::runtime_error("Change quantity outside factory");
            auto& quantity = pending ? pending->batches : room().recipe.batches;
            if (name == "MaterialCraft-MfgMinimum") quantity = 0;
            else if (name == "MaterialCraft-MfgMaximum") quantity = maximum;
            else if (name == "MaterialCraft-MfgPlus") quantity = std::min(quantity + 1, maximum);
            else throw std::runtime_error("Unexpected action: " + name);
        }
        return true;
    }
    bool confirm_manufacturing_recipe(const ManufacturingRecipe& target) {
        if (!pending || pending->item_id != target.item_id || pending->batches != target.batches)
            throw std::runtime_error("Wrong submission");
        room().recipe = target; pending.reset();
        if (target.item_id.starts_with("32")) {
            room().recipe.batches = 0; room().storage = target.batches * 5;
            saved_record = *json::open(UserDir.get() / "debug/material_craft/manufacturing_restore_123.json");
        }
        return true;
    }
    bool collect_manufacturing_product(const ManufacturingRecipe& target, int count) {
        ++collections;
        if (fail_collect) return false;
        if (room().storage != count * target.weight) throw std::runtime_error("Wrong collection");
        room().storage = 0;
        if (target.item_id.starts_with("32")) chips += count;
        return true;
    }
    int get_task_id() { return 123; }
    void callback_operation(const std::string&, const CraftOperation&, int) {}
    void manufacturing_failure(const std::string& reason, const ManufacturingRecipe& original = {}) {
        failure = reason; recovery_recipe = original;
    }
    bool ensure_manufacturing_page();
    bool select_manufacturing_recipe(const ManufacturingRecipe&);
    bool set_manufacturing_count(int);
    bool execute_manufacturing_operation(const CraftOperation&);
};
InfrastFacilityImageAnalyzer::InfrastFacilityImageAnalyzer(cv::Mat image) : h(*image.owner) {}
bool InfrastFacilityImageAnalyzer::analyze() { return !h.rooms.empty(); }
size_t InfrastFacilityImageAnalyzer::get_quantity(const char*) { return h.rooms.size(); }
Rect InfrastFacilityImageAnalyzer::get_rect(const char*, int i) { return {"", i * 100 + h.pans_to_left * 100, 300}; }
InfrastMaterialCraftImageAnalyzer::InfrastMaterialCraftImageAnalyzer(cv::Mat image) : h(*image.owner) {}
bool InfrastMaterialCraftImageAnalyzer::analyze() {
    if (h.missing_formula) return false;
    if (item.starts_with("32")) {
        const int profession = item[2] - '0';
        if (h.bottom ? profession < 3 : profession > 6) return false;
    }
    candidates = {{{item}, {item}, 1}}; return true;
}
bool ProcessTask::run() {
    if (h.stopped) return false;
    if (name == "MaterialCraft@InfrastBegin") h.page = Overview;
    return true;
}
#define LogTraceFunction
// ACTUAL_METHODS
int main() {
    int checks = 0;
    auto require = [&](bool ok) { ++checks; if (!ok) throw std::runtime_error("Check " + std::to_string(checks)); };
    const Room shard { {"3141", "30012", 37, 3} };
    const Room gold { {"3003", "", 99, 2} };
    const Room records { {"2003", "", 21, 5} };
    const CraftOperation craft { {"3283", "Mfg"}, 1 };
    const CraftOperation ordinary { {"30034", "Processing"}, 3 };
    { Harness h; h.rooms = {gold}; h.pans_to_left = 2;
      require(h.ensure_manufacturing_page()); require(h.overview_swipes == 3 && h.visits.size() == 1); }
    { Harness h; h.rooms = {gold}; h.pans_to_left = 4;
      require(!h.ensure_manufacturing_page()); require(h.visits.empty()); }
    for (bool chip_first : {false, true}) {
        Harness h; h.rooms = {gold};
        h.m_plan.operations = chip_first ? std::vector<CraftOperation>{craft, ordinary} : std::vector<CraftOperation>{ordinary, craft};
        require(h._run()); require(h.chips == 1 && h.processing_operations == 1 && h.processing_entries == 1);
        require(h.rooms[0].recipe.item_id == "3003" && h.rooms[0].recipe.batches == 99);
    }
    for (auto product : {gold, records}) {
        Harness h; h.rooms = {shard, shard, product};
        require(h.execute_manufacturing_operation(craft));
        require(h.visits == std::vector<int>({0, 1, 2}));
        require(h.rooms[0].recipe.batches == 37 && h.rooms[1].recipe.batches == 37);
        require(h.rooms[2].recipe.item_id == product.recipe.item_id && h.rooms[2].recipe.batches == product.recipe.batches);
        require(h.collections == 1 && h.chips == 1);
    }
    for (bool replenish : {false, true}) for (int quantity : {0, 1, 37, 99})
        for (const std::string ingredient : {"30012", "30062"}) for (int rooms : {1, 3}) {
            Harness h; h.rooms.assign(rooms, shard); h.rooms[0].recipe.batches = quantity;
            h.rooms[0].recipe.ingredient_id = ingredient; h.m_replenish_originium_shards = replenish;
            require(h.execute_manufacturing_operation(craft));
            require(h.rooms[0].recipe.batches == (replenish ? 99 : quantity));
            require(h.rooms[0].recipe.item_id == "3141" && h.rooms[0].recipe.ingredient_id == ingredient);
            require(h.saved_record.at("original_batches").as_integer() == quantity);
            require(h.saved_record.at("batches").as_integer() == (replenish ? 99 : quantity));
            require(!std::filesystem::exists(UserDir.get() / "debug/material_craft/manufacturing_restore_123.json"));
            require(h.collections == 1 && h.chips == 1);
            require(rooms == 1 || h.visits == std::vector<int>({0, 1, 2, 0}));
        }
    for (bool replenish : {false, true}) {
        Harness h; h.rooms = {gold}; h.m_replenish_originium_shards = replenish;
        h.rooms[0].recipe.batches = 23;
        require(h.execute_manufacturing_operation(craft)); require(h.rooms[0].recipe.batches == 23);
    }
    { Harness h; h.rooms = {shard, shard}; h.rooms[0].level_three = false;
      require(h.ensure_manufacturing_page()); require(h.m_cur_facility_index == 1); }
    for (int unavailable = 0; unavailable < 4; ++unavailable) {
        Harness h; h.rooms = {shard, gold};
        if (unavailable == 0) h.rooms[1].readable = false;
        if (unavailable == 1) h.fail_entry = 1;
        if (unavailable == 2) h.rooms[1].level_three = false;
        if (unavailable == 3) h.rooms[1].recipe.item_id = "3213";
        require(!h.execute_manufacturing_operation(craft)); require(h.chips == 0 && h.collections == 0);
        require(h.rooms[0].recipe.batches == 37);
    }
    { Harness h; require(!h.ensure_manufacturing_page()); require(h.visits.empty()); }
    { Harness h; h.rooms = {shard, gold}; h.cancel_entry = 1;
      require(!h.ensure_manufacturing_page()); require(h.collections == 0); }
    { Harness h; h.rooms = {shard}; h.m_replenish_originium_shards = true; h.fail_selection = true;
      require(!h.execute_manufacturing_operation(craft)); require(h.rooms[0].recipe.batches == 37);
      require(h.collections == 0 && h.chips == 0); }
    { Harness h; h.rooms = {shard}; h.m_replenish_originium_shards = true; h.fail_collect = true;
      require(!h.execute_manufacturing_operation(craft)); require(h.recovery_recipe.batches == 99);
      require(h.saved_record.at("original_batches").as_integer() == 37);
      require(std::filesystem::exists(UserDir.get() / "debug/material_craft/manufacturing_restore_123.json")); }
    for (bool bottom : {false, true}) for (int profession = 1; profession <= 8; ++profession) {
        Harness h; h.rooms = {gold}; h.page = Factory; h.bottom = bottom;
        const ManufacturingRecipe target {"32" + std::to_string(profession) + "3", "", 1, 5};
        require(h.select_manufacturing_recipe(target)); require(h.pending->item_id == target.item_id);
        const bool visible = bottom ? profession >= 3 : profession <= 6;
        const auto swipes = std::ranges::count(h.actions, "MaterialCraft-MfgSwipe");
        const auto rewinds = std::ranges::count(h.actions, "MaterialCraft-MfgRewind");
        require(swipes + rewinds == (visible ? 0 : 1));
        require(visible || (bottom ? rewinds : swipes) == 1);
    }
    for (const auto& product : {shard, gold, records}) {
        Harness h; h.rooms = {product}; h.page = Factory;
        require(h.select_manufacturing_recipe(product.recipe));
        require(std::ranges::count(h.actions, "MaterialCraft-MfgSwipe") == 0);
        require(std::ranges::count(h.actions, "MaterialCraft-MfgRewind") == 0);
    }
    { Harness h; h.rooms = {gold}; h.page = Factory; h.missing_formula = true;
      require(!h.select_manufacturing_recipe({"3283", "", 1, 5}));
      require(std::ranges::count(h.actions, "MaterialCraft-MfgSwipe") == 1); require(!h.pending); }
    { Harness h; h.rooms = {gold}; h.page = Factory;
      require(h.set_manufacturing_count(99)); require(h.actions == std::vector<std::string>({"MaterialCraft-MfgMaximum"})); }
    { Harness h; h.rooms = {gold}; h.page = Factory; h.maximum = 98;
      require(!h.set_manufacturing_count(99)); }
    { Harness h; h.rooms = {gold}; h.page = Factory; h.maximum = 3;
      require(!h.set_manufacturing_count(4)); }
    std::cout << "Manufacturing: " << checks << " checks passed\n";
}
"""


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    source = SOURCE.read_text(encoding="utf-8")
    methods = "\n".join(
        extract(source, name)
        for name in (
            "ensure_manufacturing_page",
            "select_manufacturing_recipe",
            "set_manufacturing_count",
            "execute_manufacturing_operation",
        )
    )
    methods += "\n" + extract(
        SOURCE.with_name("InfrastMaterialCraftTask.cpp").read_text(encoding="utf-8"),
        "_run",
    )
    (OUT / "main.cpp").write_text(
        FIXTURE.replace("// ACTUAL_METHODS", methods), encoding="utf-8"
    )
    (OUT / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.20)\nproject(CraftManufacturing LANGUAGES CXX)\n"
        "add_executable(manufacturing main.cpp)\ntarget_compile_features(manufacturing PRIVATE cxx_std_20)\n"
        "if(MSVC)\n  target_compile_options(manufacturing PRIVATE /utf-8)\nendif()\n"
        f'target_include_directories(manufacturing PRIVATE "{ROOT.as_posix()}/src/MaaUtils/include")\n',
        encoding="utf-8",
    )
    subprocess.run(["cmake", "-S", str(OUT), "-B", str(OUT / "native")], check=True)
    subprocess.run(
        ["cmake", "--build", str(OUT / "native"), "--config", "Debug"], check=True
    )
    candidates = [OUT / "native/Debug/manufacturing.exe", OUT / "native/manufacturing"]
    subprocess.run(
        [str(next(path for path in candidates if path.exists()))], check=True, cwd=OUT
    )


if __name__ == "__main__":
    main()
