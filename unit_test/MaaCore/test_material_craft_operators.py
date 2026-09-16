"""Exercise the actual workshop mood/batching methods with a simulated controller."""

from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "build/craft-operator-flow"
SOURCE = ROOT / "src/MaaCore/Task/Infrast/InfrastMaterialCraftTask.cpp"


def extract(source, name):
    start = source.index(f"InfrastMaterialCraftTask::{name}(")
    start = source.rfind("\n", 0, start) + 1
    brace = source.index("{", start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end].replace("InfrastMaterialCraftTask::", "Harness::")


FIXTURE = r"""
#include <algorithm>
#include <charconv>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include "Utils/ProcessingOperatorScore.h"
constexpr int MaxMood=24, MaxOperatorChanges=100;
namespace cv { struct Mat { int mood, cost; bool readable; }; }
struct Formula {
    std::string item_id="rock"; int ap_cost=360000;
    bool is_skill_summary() const { return item_id=="3302" || item_id=="3303"; }
};
struct CraftOperation { Formula formula; int batches; };
struct Result { std::string text; double score=1; };
struct RegionOCRer {
    cv::Mat image; std::string task;
    static inline std::optional<Result> override_result;
    RegionOCRer(cv::Mat im):image(im){}
    void set_task_info(const std::string& name) { task=name; }
    std::optional<Result> analyze() {
        if(override_result)return override_result;
        if(!image.readable)return std::nullopt;
        return Result{task=="MaterialCraft-ByproductRate"?"20%":std::to_string(image.cost)};
    }
};
struct Logger { template<class... T> void info(T&&...) {} } Log;
struct Harness {
    bool m_station_operators=true;
    bool station_before_formula=false;
    mutable int mood_reads=0;
    std::string m_scored_processing_item="rock";
    std::optional<asst::infrast::ProcessingOperatorScore> m_processing_score=asst::infrast::ProcessingOperatorScore{100,3};
    int scorings=0;
    bool rejected_current=false, wrong_skill_cost=false;
    int mood=24, cost=3, quantity=1, changes=0, starts=0, confirmed=0;
    int m_next_operation_id=0;
    bool m_inventory_complete=true, stopped=false, readable=true;
    bool cancel_on_change=false, wrong_quantity=false, invalid_count=false, fail_formula=false;
    bool extra_cost=false, infinite_candidates=false, formula_valid=true;
    bool materials_unavailable=false;
    std::vector<std::pair<int,int>> candidates;
    std::vector<int> completed;
    std::string failure;
    bool need_exit() const { return stopped; }
    Harness* ctrler() { return this; }
    const Harness* ctrler() const { return this; }
    cv::Mat get_image() const { return {mood,cost*quantity,readable}; }
    std::optional<int> read_processing_mood(const cv::Mat& im) const {
        ++mood_reads;
        return im.readable?std::optional(im.mood):std::nullopt;
    }
    std::optional<int> read_craft_count() const { return quantity; }
    bool open_formula_selector(const Formula* next) { station_before_formula=next!=nullptr;return !stopped; }
    void update_processing_operator_mood(int) {}
    bool selected_formula_matches(const Formula&) { return formula_valid&&!stopped; }
    bool select_formula(const Formula&) { quantity=materials_unavailable?0:wrong_quantity?2:1;return formula_valid&&!stopped; }
    bool next_candidate() {
        ++changes;
        if(cancel_on_change){stopped=true;return false;}
        if(infinite_candidates){mood=1;cost=25;return true;}
        if(candidates.empty())return false;
        mood=candidates.front().first;cost=candidates.front().second;
        candidates.erase(candidates.begin());
        if(fail_formula)formula_valid=false;
        return true;
    }
    bool select_processing_operator(const Formula& f,std::vector<std::string>&,int current_mood) {
        ++scorings;rejected_current=current_mood<=0 || current_mood<cost;
        if(!next_candidate())return false;
        m_scored_processing_item=f.item_id;
        m_processing_score=asst::infrast::ProcessingOperatorScore{80,wrong_skill_cost?cost+1:cost};
        return true;
    }
    std::optional<int> set_craft_count(int count) {
        quantity=invalid_count?count+1:count;
        if(extra_cost)cost+=24;
        return quantity;
    }
    void callback_operation(const std::string&,const CraftOperation&,int) {}
    bool click_start_button() { if(stopped)return false;++starts;return true; }
    bool click_complete_tick(const CraftOperation& actual,int) {
        if(m_station_operators&&cost*actual.batches>mood)throw std::runtime_error("Overloaded craft");
        mood-=cost*actual.batches;confirmed+=actual.batches;completed.push_back(actual.batches);return true;
    }
    void processing_operator_failure(const std::string& why) { if(!stopped)failure=why; }
    std::optional<int> read_processing_number(const cv::Mat&,const std::string&,bool fraction=false) const;
    std::optional<int> prepare_processing_operator(const Formula&,int);
    bool processing_mood_sufficient() const;
    bool execute_operation(const CraftOperation&);
};
// ACTUAL_METHODS
int main() {
    int checks=0;
    auto require=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Check "+std::to_string(checks));};
    // Disabled stationing follows the legacy quantity flow even without readable operator metadata.
    { Harness h;h.m_station_operators=false;h.mood=0;h.readable=false;
      require(h.execute_operation({{},9}));require(h.completed==std::vector<int>{9});
      require(h.changes==0&&h.scorings==0&&h.mood_reads==0&&!h.station_before_formula);
      require(h.confirmed==9&&!h.m_inventory_complete); }
    { Harness h;h.m_station_operators=false;h.formula_valid=false;
      require(!h.execute_operation({{},1}));require(h.starts==0&&h.changes==0); }
    { Harness h;h.m_station_operators=false;h.invalid_count=true;
      require(!h.execute_operation({{},1}));require(h.starts==0&&h.changes==0); }
    { Harness h;h.m_station_operators=false;h.stopped=true;
      require(!h.execute_operation({{},1}));require(h.starts==0&&h.changes==0); }
    // Re-enabling stationing restores mood checks and operator replacement.
    { Harness h;h.m_station_operators=false;
      require(h.execute_operation({{},8}));require(h.mood==0&&h.mood_reads==0);
      h.m_station_operators=true;h.candidates={{24,2}};
      require(h.execute_operation({{},1}));
      require(h.station_before_formula&&h.changes==1&&h.mood==22&&h.mood_reads>0); }
    { Harness h; h.candidates={{24,1}};
      require(h.execute_operation({{},9}));require(h.completed==std::vector<int>{8,1});
      require(h.changes==1&&h.mood==23&&h.confirmed==9);require(!h.m_inventory_complete); }
    { Harness h; require(h.execute_operation({{},8}));require(h.changes==0&&h.mood==0); }
    { Harness h; h.mood=0;h.candidates={{24,2}};
      require(h.execute_operation({{},2}));require(h.changes==1&&h.mood==20); }
    { Harness h; h.mood=2;h.candidates={{24,4}};
      require(h.execute_operation({{},3}));require(h.changes==1&&h.mood==12); }
    { Harness h; h.mood=0;h.candidates={{1,3},{24,4}};
      require(h.execute_operation({{},6}));require(h.changes==2&&h.mood==0); }
    { Harness h; h.cost=0;h.m_processing_score->mood_cost=0;
      require(h.execute_operation({{},99}));require(h.mood==24&&h.confirmed==99&&h.changes==0); }
    { Harness h; h.mood=0;h.cost=0;
      require(!h.execute_operation({{},1}));require(h.starts==0&&h.failure=="OperatorSelectionFailed"); }
    { Harness h; h.mood=0;
      require(!h.execute_operation({{},1}));require(h.starts==0&&h.confirmed==0); }
    { Harness h; h.candidates.clear();
      require(!h.execute_operation({{},9}));require(h.completed==std::vector<int>{8}); }
    { Harness h; h.readable=false;
      require(!h.execute_operation({{},1}));require(h.changes==0&&h.starts==0&&h.failure=="MoodRecognitionFailed"); }
    { Harness h; h.wrong_quantity=true;
      require(!h.execute_operation({{},1}));require(h.starts==0); }
    { Harness h; h.materials_unavailable=true;
      require(!h.execute_operation({{},1}));require(h.starts==0&&h.changes==0&&h.failure=="MaterialsUnavailable"); }
    { Harness h; h.invalid_count=true;
      require(!h.execute_operation({{},1}));require(h.starts==0); }
    { Harness h; h.extra_cost=true;
      require(!h.execute_operation({{},1}));require(h.starts==0&&h.failure=="MoodVerificationFailed"); }
    { Harness h; h.stopped=true;
      require(!h.execute_operation({{},1}));require(h.starts==0&&h.changes==0); }
    { Harness h; h.mood=0;h.cancel_on_change=true;
      require(!h.execute_operation({{},1}));require(h.starts==0&&h.changes==1&&h.failure.empty()); }
    { Harness h; h.mood=0;h.fail_formula=true;h.candidates={{24,1}};
      require(!h.execute_operation({{},1}));require(h.starts==0&&h.changes==1); }
    { Harness h; h.mood=0;h.infinite_candidates=true;
      require(!h.execute_operation({{},1}));require(h.changes==MaxOperatorChanges&&h.starts==0); }
    { Harness h; h.cost=1;h.m_processing_score->mood_cost=1;
      require(h.prepare_processing_operator({},std::numeric_limits<int>::max())==24); }
    // A healthy incumbent still needs scoring on the first book recipe (including an excluded incumbent).
    { Harness h; h.cost=2;h.candidates={{24,2}};
      require(h.execute_operation({{"3303"},2}));require(h.scorings==1&&!h.rejected_current);
      require(h.confirmed==2&&h.mood==20); }
    // Reuse the verified operator for the same recipe; changing book recipes must recompute mood tie-breaks.
    { Harness h; h.cost=1;h.candidates={{24,1},{24,2}};
      require(h.prepare_processing_operator({"3302"},99)==24);
      require(h.prepare_processing_operator({"3302"},99)==24);require(h.scorings==1);
      require(h.prepare_processing_operator({"3303"},99)==12);require(h.scorings==2); }
    { Harness h;h.mood=1;h.cost=2;h.candidates={{24,1}};
      require(h.execute_operation({{"3303"},2}));require(h.rejected_current&&h.scorings==1); }
    { Harness h;h.cost=2;h.candidates={{24,2}};h.wrong_skill_cost=true;
      require(!h.execute_operation({{"3303"},1}));
      require(h.starts==0&&h.failure=="SkillVerificationFailed"); }
    { Harness h;h.cost=2;h.candidates={{24,2},{24,1}};
      require(h.execute_operation({{"3303"},13}));
      require(h.completed==std::vector<int>{12,1}&&h.scorings==2&&h.rejected_current); }
    // Elite materials use the same initial ranking, recipe change and cost verification.
    { Harness h;h.candidates={{24,3},{24,6}};
      require(h.prepare_processing_operator({"31024"},99)==8);
      require(h.prepare_processing_operator({"31024"},99)==8);require(h.scorings==1);
      require(h.prepare_processing_operator({"30044"},99)==4);require(h.scorings==2); }
    { Harness h;h.candidates={{24,3}};h.wrong_skill_cost=true;
      require(!h.execute_operation({{"31024"},1}));
      require(h.starts==0&&h.failure=="SkillVerificationFailed"); }
    { Harness h;h.candidates={{24,10},{24,3}};
      require(h.execute_operation({{"30145"},3}));
      require(h.completed==std::vector<int>{2,1}&&h.scorings==2&&h.rejected_current); }
    for(const auto text:{"0/24","1/24","24/24"}) {
        RegionOCRer::override_result=Result{text};Harness h;
        require(h.read_processing_number(h.get_image(),"mood",true).has_value());
    }
    for(const auto text:{"25/24","24/25","-1/24","1/24x","/24","24","2a/24"}) {
        RegionOCRer::override_result=Result{text};Harness h;
        require(!h.read_processing_number(h.get_image(),"mood",true));
    }
    for(const auto text:{"-1","3x","99999999999999999999999",""}) {
        RegionOCRer::override_result=Result{text};Harness h;
        require(!h.read_processing_number(h.get_image(),"cost"));
    }
    { RegionOCRer::override_result=Result{"24/24",.89};Harness h;
      require(!h.read_processing_number(h.get_image(),"mood",true)); }
    std::cout<<"Workshop operators: "<<checks<<" checks passed\n";
}
"""


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    operators = SOURCE.with_name("InfrastMaterialCraftTask_Operators.cpp").read_text(
        encoding="utf-8"
    )
    methods = "\n".join(
        extract(operators, name)
        for name in (
            "read_processing_number",
            "prepare_processing_operator",
            "processing_mood_sufficient",
        )
    )
    methods += "\n" + extract(SOURCE.read_text(encoding="utf-8"), "execute_operation")
    (OUT / "main.cpp").write_text(
        FIXTURE.replace("// ACTUAL_METHODS", methods), encoding="utf-8"
    )
    (OUT / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.20)\nproject(CraftOperators LANGUAGES CXX)\n"
        "add_executable(operators main.cpp)\n"
        f'target_include_directories(operators PRIVATE "{ROOT.as_posix()}/src/MaaCore")\n'
        "target_compile_features(operators PRIVATE cxx_std_20)\n",
        encoding="utf-8",
    )
    subprocess.run(["cmake", "-S", str(OUT), "-B", str(OUT / "native")], check=True)
    subprocess.run(
        ["cmake", "--build", str(OUT / "native"), "--config", "Debug"], check=True
    )
    candidates = [OUT / "native/Debug/operators.exe", OUT / "native/operators"]
    subprocess.run(
        [str(next(path for path in candidates if path.exists()))], check=True
    )


if __name__ == "__main__":
    main()
