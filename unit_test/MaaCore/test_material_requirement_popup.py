"""Run actual requirement popup control flow with simulated frames; no game I/O."""

from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "build/requirement-popup-check"


def extract(source, name):
    token = f"asst::MaterialRequirementRecognitionTask::{name}("
    pos = source.index(token)
    start = source.rfind("\n", 0, pos) + 1
    end = source.index("{", pos) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end].replace(
        "asst::MaterialRequirementRecognitionTask::", "Harness::"
    )


FIXTURE = r"""
#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#define LogTraceFunction
namespace utils { using path = std::filesystem::path; }
struct Rect { int slot = -1; bool operator==(const Rect&) const = default; };
struct MaterialRequirementInfo {
    std::string item_id; int owned = 0, required = 4, shortage = 4;
    Rect item_rect, quantity_rect;
};
enum Page { Promotion, Mastery, Popup, Other };
struct Harness;
struct Image { Harness* owner; };
struct Config { Rect specific_rect{-1}; };
struct { std::shared_ptr<Config> get(const char*) { return std::make_shared<Config>(); } } Task;
struct MaterialRequirementImageAnalyzer {
    Harness& h;
    std::vector<MaterialRequirementInfo> items, materials;
    std::optional<MaterialRequirementInfo> chip;
    bool quantities_complete = true;
    MaterialRequirementImageAnalyzer(Image image) : h(*image.owner) {}
    MaterialRequirementImageAnalyzer& operator=(MaterialRequirementImageAnalyzer&& other) {
        items = std::move(other.items); materials = std::move(other.materials);
        chip = std::move(other.chip); quantities_complete = other.quantities_complete; return *this;
    }
    void set_cancel_check(std::function<bool()>) {}
    bool analyze(bool);
    bool complete() { return quantities_complete && !chip && materials.empty(); }
    const auto& get_result() { return items; }
    const auto& pending_chip() { return chip; }
    const auto& pending_materials() { return materials; }
    bool confirm_chip_name(const std::string& name) {
        if (!chip || name != "Vanguard") return false;
        chip->item_id = "3213"; items.insert(items.begin(), *chip); chip.reset(); return true;
    }
    bool confirm_material_name(const MaterialRequirementInfo& pending, const std::string& name) {
        auto it=std::ranges::find_if(materials,[&](const auto& i){
            return i.quantity_rect==pending.quantity_rect && i.owned==pending.owned && i.required==pending.required;
        });
        if(it==materials.end())return false;
        if(name!="Grindstone" && name!="Device")return false;
        it->item_id=name=="Grindstone"?"30094":"30064";items.push_back(*it);materials.erase(it);return true;
    }
    bool recognize_material_icon(const MaterialRequirementInfo&);
    static bool is_promotion_page(Image);
    static bool has_item_popup(Image);
    static bool is_requirement_page(Image);
    static std::optional<std::string> read_popup_name(Image);
};
struct Harness {
    Page page = Promotion, return_page = Promotion;
    bool stopped=false, cancel_sleep=false, close_fails=false, open_fails=false, close_click_fails=false;
    bool fallback_success=true, quantities_complete=true, change_page=false, open_noop=false;
    int changed_slot=-1, cancel_at_read=-1, active_slot=0, opens=0, closes=0, reads=0, analyses=0, fallbacks=0;
    std::vector<int> opened_slots;
    std::vector<MaterialRequirementInfo> slots = {
        {"3213",0,4,4,{0},{0}}, {"30094",7,7,0,{1},{1}}, {"30064",6,6,0,{2},{2}}
    };
    std::map<int,std::vector<std::optional<std::string>>> names={
        {0,{"Vanguard","Vanguard","Vanguard"}},
        {1,{"Grindstone","Grindstone","Grindstone"}},
        {2,{"Device","Device","Device"}}
    };
    std::map<int,size_t> read_positions;
    std::string status;
    std::vector<MaterialRequirementInfo> m_result;
    bool need_exit() { return stopped; }
    Harness* ctrler() { return this; }
    Image get_image() { return {this}; }
    bool sleep(int) { if(cancel_sleep)stopped = true; return !stopped; }
    bool click(Rect r) {
        if (stopped) throw std::runtime_error("Click after stop");
        if (r.slot<0) {
            ++closes;
            if(close_click_fails)return false;
            if(!close_fails)page=change_page?Mastery:return_page;
            if(changed_slot>=0){++slots[changed_slot].owned;changed_slot=-1;}
        }
        else {
            ++opens;opened_slots.push_back(r.slot);
            if(page==Popup)throw std::runtime_error("Opened through a blocked popup");
            if(open_fails)return false;
            if(open_noop)return true;
            active_slot=r.slot;page=Popup;
        }
        return true;
    }
    void callback_analyze_result(const std::string& value) { status = value; }
    void save_img(const std::filesystem::path&) {}
    void missing(int slot) { slots[slot].owned=slot==0?0:4; }
    void sufficient() { for(auto& slot:slots)slot.owned=slot.required; }
    bool _run();
    bool close_item_popup();
    std::optional<std::string> verify_item_name(const Rect&);
};
bool MaterialRequirementImageAnalyzer::has_item_popup(Image i) { return i.owner->page==Popup; }
bool MaterialRequirementImageAnalyzer::is_promotion_page(Image i) { return i.owner->page==Promotion; }
bool MaterialRequirementImageAnalyzer::is_requirement_page(Image i) { return i.owner->page==Promotion || i.owner->page==Mastery; }
std::optional<std::string> MaterialRequirementImageAnalyzer::read_popup_name(Image i) {
    auto& h=*i.owner;if(h.page!=Popup)return std::nullopt;
    if(++h.reads==h.cancel_at_read)h.stopped=true;
    auto& pos=h.read_positions[h.active_slot];auto& list=h.names[h.active_slot];
    return pos<list.size()?list[pos++]:std::nullopt;
}
bool MaterialRequirementImageAnalyzer::analyze(bool deferred) {
    if(!deferred)throw std::runtime_error("Unexpected full scan");
    ++h.analyses;quantities_complete=h.quantities_complete;
    if(!is_requirement_page({&h})){quantities_complete=false;return false;}
    for(auto item:h.slots) {
        if(item.owned>=item.required)continue;
        item.shortage=item.required-item.owned;
        if(item.item_rect.slot==0) {
            if(h.page==Promotion)chip=item;else {item.item_id="3303";items.push_back(item);}
        }
        else {item.item_id.clear();materials.push_back(item);}
    }
    return true;
}
bool MaterialRequirementImageAnalyzer::recognize_material_icon(const MaterialRequirementInfo& pending) {
    ++h.fallbacks;if(!h.fallback_success)return false;
    return confirm_material_name(pending,pending.item_rect.slot==1?"Grindstone":"Device");
}
// METHODS
int main() {
    int checks=0;
    auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Check "+std::to_string(checks));};
    for(Page page:{Promotion,Mastery})for(bool opened:{false,true})for(int mask=0;mask<4;++mask) {
        Harness h;h.sufficient();h.page=h.return_page=page;
        if(mask&1)h.missing(1);if(mask&2)h.missing(2);
        if(opened){h.page=Popup;h.active_slot=1;}
        check(h._run());check(h.status=="success");
        const int missing=bool(mask&1)+bool(mask&2);
        check(h.m_result.size()==missing);check(h.opens==missing);
        check(h.closes==missing+opened && h.page==page);
        check(h.fallbacks==0 && h.analyses==(missing?2:1));
    }
    { Harness h;h.missing(1);h.missing(2);check(h._run());
      check(h.m_result.size()==3 && h.m_result[0].item_id=="3213");
      check(h.opened_slots==std::vector<int>({0,1,2}));check(h.analyses==2 && h.fallbacks==0); }
    { Harness h;h.page=h.return_page=Mastery;check(h._run());
      check(h.status=="success" && h.m_result[0].item_id=="3303" && h.opens==0); }
    { Harness h;h.sufficient();h.missing(1);h.missing(2);h.names[1]={};check(h._run());
      check(h.status=="success" && h.m_result.size()==2 && h.fallbacks==1);check(h.closes==2); }
    { Harness h;h.sufficient();h.missing(1);h.names[1]={};h.fallback_success=false;check(h._run());
      check(h.status=="partial" && h.m_result.empty() && h.fallbacks==1); }
    { Harness h;h.sufficient();h.missing(1);h.names[1]={"Vanguard","Vanguard"};check(h._run());
      check(h.status=="partial" && h.m_result.empty() && h.fallbacks==0); }
    { Harness h;h.missing(1);h.names[0]={};check(h._run());
      check(h.status=="partial" && h.m_result.size()==1 && h.m_result[0].item_id=="30094");check(h.fallbacks==0); }
    { Harness h;h.names[0]={"Vanguard","Guard","Vanguard"};check(h._run());
      check(h.status=="partial" && h.m_result.empty() && h.reads==2); }
    { Harness h;h.sufficient();h.missing(1);h.names[1]={std::nullopt,"Grindstone","Grindstone"};check(h._run());
      check(h.status=="success" && h.fallbacks==0); }
    for(int slot:{0,1}) {
        Harness h;h.sufficient();h.missing(slot);h.changed_slot=slot;check(h._run());
        check(h.status=="partial" && h.m_result.empty());
    }
    for(int failure=0;failure<3;++failure) {
        Harness h;h.missing(1);h.close_fails=failure==0;h.close_click_fails=failure==1;h.change_page=failure==2;
        check(!h._run());check(h.status=="failed" && h.m_result.empty());check(h.opens==1 && h.analyses==1);
    }
    { Harness h;h.sufficient();h.missing(1);h.open_fails=true;check(h._run());
      check(h.status=="success" && h.fallbacks==1 && h.closes==0); }
    { Harness h;h.sufficient();h.missing(1);h.open_noop=true;check(h._run());
      check(h.status=="success" && h.fallbacks==1 && h.closes==0); }
    { Harness h;h.page=Popup;h.active_slot=1;h.names[1]={};h.sufficient();check(h._run());
      check(h.status=="success" && h.closes==1); }
    { Harness h;h.cancel_sleep=true;check(!h._run());check(h.status.empty() && h.closes==0); }
    { Harness h;h.cancel_at_read=1;check(!h._run());check(h.status.empty() && h.closes==0); }
    { Harness h;h.stopped=true;check(!h._run());check(h.opens==0 && h.status.empty()); }
    { Harness h;h.sufficient();h.quantities_complete=false;check(h._run());
      check(h.status=="partial" && h.m_result.empty() && h.opens==0); }
    { Harness h;h.page=Other;check(!h._run());check(h.status=="failed" && h.opens==0 && h.closes==0); }
    std::cout<<"Requirement popup: "<<checks<<" checks passed\n";
}
"""


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    source = (
        ROOT / "src/MaaCore/Task/Miscellaneous/MaterialRequirementRecognitionTask.cpp"
    ).read_text(encoding="utf-8")
    methods = "\n".join(
        extract(source, name)
        for name in ("_run", "close_item_popup", "verify_item_name")
    )
    (OUT / "main.cpp").write_text(
        FIXTURE.replace("// METHODS", methods), encoding="utf-8"
    )
    (OUT / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.20)\nproject(RequirementPopup LANGUAGES CXX)\n"
        "add_executable(popup main.cpp)\ntarget_compile_features(popup PRIVATE cxx_std_20)\n",
        encoding="utf-8",
    )
    subprocess.run(["cmake", "-S", str(OUT), "-B", str(OUT / "native")], check=True)
    subprocess.run(
        ["cmake", "--build", str(OUT / "native"), "--config", "Debug"], check=True
    )
    candidates = [OUT / "native/Debug/popup.exe", OUT / "native/popup"]
    subprocess.run([str(next(p for p in candidates if p.exists()))], check=True)


if __name__ == "__main__":
    main()
