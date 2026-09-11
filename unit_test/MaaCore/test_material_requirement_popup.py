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
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#define LogTraceFunction
namespace utils { using path = std::filesystem::path; }
struct Rect { bool close = false; };
struct MaterialRequirementInfo { std::string item_id; int owned = 0, required = 4, shortage = 4; Rect item_rect; };
enum Page { Promotion, Popup, Other };
struct Harness;
struct Image { Harness* owner; };
struct Config { Rect specific_rect{true}; };
struct { std::shared_ptr<Config> get(const char*) { return std::make_shared<Config>(); } } Task;
struct MaterialRequirementImageAnalyzer {
    Harness& h;
    std::vector<MaterialRequirementInfo> items;
    std::optional<MaterialRequirementInfo> pending;
    bool done = false;
    MaterialRequirementImageAnalyzer(Image image) : h(*image.owner) {}
    MaterialRequirementImageAnalyzer& operator=(MaterialRequirementImageAnalyzer&& other) {
        items = std::move(other.items); pending = std::move(other.pending); done = other.done; return *this;
    }
    void set_cancel_check(std::function<bool()>) {}
    bool analyze();
    bool complete() { return done; }
    const auto& get_result() { return items; }
    const auto& pending_chip() { return pending; }
    bool confirm_chip_name(const std::string& name) {
        if (!pending || name != "Vanguard") return false;
        pending->item_id = "3213"; items.insert(items.begin(), *pending); pending.reset(); done = true; return true;
    }
    static bool is_promotion_page(Image);
    static std::optional<std::string> read_chip_popup_name(Image);
};
struct Harness {
    Page page = Promotion;
    bool stopped = false, cancel_sleep = false, close_fails = false, other_missing = false, changed_count = false;
    int owned = 0, opens = 0, closes = 0, reads = 0;
    std::vector<std::optional<std::string>> names = {"Vanguard", "Vanguard", "Vanguard"};
    std::string status;
    std::vector<MaterialRequirementInfo> m_result;
    bool need_exit() { return stopped; }
    Harness* ctrler() { return this; }
    Image get_image() { return {this}; }
    bool sleep(int) { if(cancel_sleep)stopped = true; return !stopped; }
    bool click(Rect r) {
        if (stopped) throw std::runtime_error("Click after stop");
        if (r.close) { ++closes; if(!close_fails)page = Promotion; if(changed_count)owned = 1; }
        else { ++opens; page = Popup; }
        return true;
    }
    void callback_analyze_result(const std::string& value) { status = value; }
    void save_img(const std::filesystem::path&) {}
    bool _run();
    bool close_chip_popup();
    std::optional<std::string> verify_chip_name(const Rect&);
};
bool MaterialRequirementImageAnalyzer::is_promotion_page(Image i) { return i.owner->page == Promotion; }
std::optional<std::string> MaterialRequirementImageAnalyzer::read_chip_popup_name(Image i) {
    auto& h = *i.owner;
    if(h.page != Popup)return std::nullopt;
    return h.reads < h.names.size() ? h.names[h.reads++] : std::nullopt;
}
bool MaterialRequirementImageAnalyzer::analyze() {
    if(h.page != Promotion)return false;
    if(h.owned < 4)pending = MaterialRequirementInfo{"", h.owned, 4, 4-h.owned};
    if(h.other_missing)items.push_back({"30034", 4, 7, 3});
    done = !pending; return true;
}
// METHODS
int main() {
    int checks = 0;
    auto check = [&](bool ok){++checks;if(!ok)throw std::runtime_error("Check " + std::to_string(checks));};
    for(bool open : {false,true}) {
        Harness h; if(open)h.page = Popup;
        check(h._run());check(h.status == "success" && h.m_result.size() == 1);
        check(h.opens == 1 && h.closes == (open?2:1) && h.page == Promotion);
    }
    { Harness h; h.other_missing = true; check(h._run());
      check(h.m_result.size() == 2 && h.m_result[0].item_id == "3213" && h.m_result[1].shortage == 3); }
    { Harness h; h.names = {}; h.other_missing = true; check(h._run());
      check(h.status == "partial" && h.m_result.size() == 1 && h.m_result[0].item_id == "30034"); check(h.closes == 1); }
    { Harness h; h.names = {"Vanguard", "Guard", "Vanguard"}; check(h._run());
      check(h.status == "partial" && h.m_result.empty()); }
    { Harness h; h.names = {std::nullopt, "Vanguard", "Vanguard"}; check(h._run());check(h.status == "success"); }
    { Harness h; h.close_fails = true; check(!h._run()); check(h.status == "failed" && h.m_result.empty()); }
    { Harness h; h.changed_count = true; check(h._run());check(h.status == "partial" && h.m_result.empty()); }
    { Harness h; h.cancel_sleep = true; check(!h._run());check(h.status.empty() && h.closes == 0); }
    { Harness h; h.owned = 4; check(h._run());check(h.status == "success" && h.opens == 0 && h.closes == 0); }
    { Harness h; h.stopped = true; check(!h._run());check(h.opens == 0 && h.status.empty()); }
    std::cout << "Requirement popup: " << checks << " checks passed\n";
}
"""


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    source = (
        ROOT / "src/MaaCore/Task/Miscellaneous/MaterialRequirementRecognitionTask.cpp"
    ).read_text(encoding="utf-8")
    methods = "\n".join(
        extract(source, name)
        for name in ("_run", "close_chip_popup", "verify_chip_name")
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
