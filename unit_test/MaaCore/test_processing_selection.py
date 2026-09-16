"""Exercise the actual multi-page processing selector with recognized operator snapshots."""

from pathlib import Path
import subprocess

from test_material_craft_operators import extract

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "build/processing-selection-test"

FIXTURE = r"""
#include "Utils/ProcessingOperatorScore.h"
#include <algorithm>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <vector>
namespace asst::infrast {
enum class Doing { Invalid=-1, Nothing, Resting, Working };
struct Skill { std::string id; };
struct Oper {
    std::string operator_id,face_hash;
    std::vector<Skill> skills;
    std::unordered_set<std::string> operator_ids;
    double mood_ratio=1;
    bool selected=false;
    Doing doing=Doing::Invalid;
};
}
using namespace asst;
struct Formula : MaterialFormula {
    Formula(std::string item="3303",int mood=2) {
        item_id=item;ap_cost=mood*360000;buff_type=is_skill_summary()?"W_SKILL":"W_EVOLVE";
    }
};
struct Image { std::vector<infrast::Oper> opers; };
struct InfrastOperImageAnalyzer {
    static constexpr int All=0;
    Image image;
    InfrastOperImageAnalyzer(Image im):image(std::move(im)){}
    void set_to_be_calced(int){}
    void set_facility(const char*){}
    bool analyze(){return !image.opers.empty();}
    void sort_by_loc(){}
    const auto& get_result()const{return image.opers;}
    int get_num_of_opers_with_skills()const {
        return std::ranges::count_if(image.opers,[](const auto& o){return !o.skills.empty();});
    }
};
struct ProcessTask {
    template<class T> ProcessTask(T&,std::initializer_list<const char*>){ }
    bool run(){return true;}
};
struct Logger { template<class... T> void info(T&&...){} } Log;
constexpr int MaxOperatorPages=100;
bool contains_face(const std::vector<std::string>& faces,const std::string& face) {
    return std::ranges::find(faces,face)!=faces.end();
}
std::unordered_set<std::string> skill_ids(const infrast::Oper& oper) {
    std::unordered_set<std::string> ids;for(const auto& s:oper.skills)ids.emplace(s.id);return ids;
}
struct Harness {
    std::vector<std::vector<infrast::Oper>> pages;
    size_t page=0;
    std::string chosen,m_scored_processing_item;
    std::optional<infrast::ProcessingOperatorScore> m_processing_score;
    bool stopped=false, fail_locate=false;
    int confirmations=0,entries=0,images=0,locations=0;
    struct ProcessingCandidate { infrast::Oper oper; std::optional<int> mood; };
    std::vector<ProcessingCandidate> m_processing_candidates;
    bool m_processing_candidates_scanned=false;
    std::optional<size_t> m_processing_operator;
    std::optional<bool> m_stainless_in_dorm;
    bool need_exit()const{return stopped;}
    Harness* ctrler(){return this;}
    Image get_image(){++images;return {pages.at(page)};}
    bool is_craft_page(const Image&){return true;}
    void close_quick_formation_expand_role(){}
    void swipe_to_the_left_of_operlist(){page=0;}
    void swipe_of_operlist(){page=std::min(page+1,pages.size()-1);}
    bool resolve_operator_identity(infrast::Oper&){return false;}
    bool locate_processing_operator(const infrast::Oper& target){++locations;chosen=target.face_hash;return !fail_locate;}
    bool confirm_processing_operator(){++confirmations;return !stopped;}
    bool enter_processing_operator_list(){++entries;page=0;return !stopped;}
    bool scan_processing_operators();
    void update_processing_operator_mood(int mood);
    bool select_processing_operator(const Formula&,std::vector<std::string>&,int,bool replace_current=false);
};
// ACTUAL_METHODS
infrast::Oper oper(std::string id,std::initializer_list<const char*> skills,bool selected=false) {
    infrast::Oper o;o.operator_id=id;o.face_hash=id;o.selected=selected;
    for(auto s:skills)o.skills.push_back({s});return o;
}
int main() {
    int checks=0;
    auto require=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Check "+std::to_string(checks));};
    auto run=[](Harness& h,bool reject=false){std::vector<std::string> rejected;return h.select_processing_operator({},rejected,reject?0:24);};
    auto feather=oper("char_421_crow",{"bskill_ws_skill2","bskill_ws_skill_cost1"},true);
    auto high=oper("char_188_helage",{"bskill_ws_skill3"});
    auto end=oper("end",{});
    // The shared recognizer leaves non-working status Invalid. It must remain eligible.
    { Harness h;h.pages={{feather},{high},{end}};
      require(run(h));require(h.chosen==high.face_hash);require(h.m_processing_score->bonus_percent==80); }
    // A full-mood Nine-Colored Deer incumbent is excluded even if it is selected.
    { auto deer=oper("char_4019_ncdeer",{"bskill_ws_bonus1","bskill_ws_bonus2"},true);
      Harness h;h.pages={{deer},{high},{end}};require(run(h));require(h.chosen==high.face_hash); }
    { auto busy=high;busy.doing=infrast::Doing::Working;
      Harness h;h.pages={{feather,busy},{end}};require(run(h));require(h.chosen==feather.face_hash); }
    { auto current=high;current.selected=true;current.doing=infrast::Doing::Working;
      Harness h;h.pages={{feather,current},{end}};require(run(h));require(h.chosen==current.face_hash); }
    { auto equal=oper("other",{"bskill_ws_skill3"});auto current=high;current.selected=true;
      Harness h;h.pages={{equal},{current},{end}};require(run(h));require(h.chosen==current.face_hash); }
    { auto equal=oper("other",{"bskill_ws_skill2"},true);feather.selected=false;
      Harness h;h.pages={{equal},{feather},{end}};require(run(h));require(h.chosen==feather.face_hash); }
    { auto current=high;current.selected=true;
      Harness h;h.pages={{current,feather},{end}};require(run(h,true));require(h.chosen==feather.face_hash); }
    // An overloaded incumbent must be replaced even if a cheaper next recipe would make it eligible.
    { auto current=high;current.selected=true;
      Harness h;h.pages={{current,feather},{end}};std::vector<std::string> rejected;
      require(h.select_processing_operator({},rejected,2,true));
      require(h.chosen==feather.face_hash);require(h.m_processing_candidates[0].mood==2); }
    { auto zero=high;zero.mood_ratio=0;
      Harness h;h.pages={{zero},{end}};require(!run(h));require(h.confirmations==0); }
    { auto unknown=oper("",{"bskill_ws_p2"});unknown.face_hash="unknown";
      unknown.operator_ids={"char_458_rfrost","char_226_hmau"};
      Harness h;h.pages={{unknown},{end}};require(!run(h));require(h.confirmations==0); }
    { Harness h;h.pages={{high},{end}};h.stopped=true;
      require(!run(h));require(h.confirmations==0); }
    { Harness h;h.pages={{high},{end}};h.fail_locate=true;
      require(!run(h));require(h.confirmations==0&&!h.m_processing_score); }
    { Harness h;h.pages={{high},{end}};std::vector<std::string> rejected{high.face_hash};
      require(!h.select_processing_operator({},rejected,24));require(h.confirmations==0); }
    auto nian=oper("char_2014_nian",{"bskill_ws_evolve4","bskill_ws_nian"},true);
    auto horn=oper("char_4039_horn",{"bskill_ws_alloyblock","bskill_ws_evolve_cost1"});
    auto elite=[](Harness& h,const char* item,int mood) {
      std::vector<std::string> rejected;return h.select_processing_operator({item,mood},rejected,24);
    };
    { Harness h;h.pages={{nian},{horn},{end}};
      require(elite(h,"31024",4));require(h.chosen==horn.face_hash);
      require(h.m_processing_score->bonus_percent==100&&h.m_processing_score->mood_cost==3); }
    { Harness h;h.pages={{horn},{nian},{end}};
      require(elite(h,"30044",4));require(h.chosen==nian.face_hash); }
    { auto bough=oper("char_4207_branch",{"bskill_ws_oriron","bskill_ws_p8"});
      auto toddi=oper("char_363_toddi",{"bskill_ws_oriron","bskill_ws_evolve_cost1"});
      Harness h;h.pages={{bough},{toddi},{end}};
      require(elite(h,"30043",2));require(h.chosen==bough.face_hash);
      require(elite(h,"30044",4));require(h.chosen==toddi.face_hash); }
    { auto ambiguous=oper("",{"bskill_ws_oriron"});ambiguous.face_hash="iron";
      ambiguous.operator_ids={"char_4207_branch","char_363_toddi"};
      Harness h;h.pages={{ambiguous},{end}};
      require(!elite(h,"30044",4));require(h.confirmations==0);
      require(elite(h,"30043",2));require(h.m_processing_score->bonus_percent==90); }
    { auto dynamic=oper("",{"bskill_ws_evolve_dorm2","bskill_ws_cost&dorm"});dynamic.face_hash="dynamic";
      Harness h;h.pages={{dynamic},{nian},{end}};
      require(elite(h,"30044",4));require(h.chosen==nian.face_hash); }
    { Harness h;h.pages={{high},{end}};require(!elite(h,"30044",4));require(h.confirmations==0); }
    // A book-first scan must retain elite-only operators for later materials.
    { Harness h;h.pages={{high},{nian,horn},{end}};
      require(run(h));require(h.chosen==high.face_hash);const int scanned=h.images;
      require(elite(h,"31024",4));require(h.chosen==horn.face_hash);require(h.images==scanned);
      const int entries=h.entries,locations=h.locations;
      require(elite(h,"31024",4));require(h.entries==entries&&h.locations==locations);
      require(elite(h,"30014",4));require(h.chosen==nian.face_hash);require(h.images==scanned); }
    // Persist an exact insufficient mood value across materials; do not keep trying the same operator.
    { Harness h;h.pages={{nian},{horn,oper("fallback",{"bskill_ws_evolve3"})},{end}};
      require(elite(h,"30014",4));require(h.chosen==nian.face_hash);
      std::vector<std::string> rejected;
      require(h.select_processing_operator({"30014",4},rejected,2));require(h.chosen=="fallback");
      require(elite(h,"30145",8));require(h.chosen=="fallback");require(h.entries==2);
      require(h.m_processing_candidates[0].mood==2); }
    // A failed scan must not publish a partial cache.
    { Harness h;h.pages={{}};require(!run(h));require(!h.m_processing_candidates_scanned); }
    std::cout<<"Processing selection: "<<checks<<" checks passed\n";
}
"""


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    source = (
        ROOT / "src/MaaCore/Task/Infrast/InfrastMaterialCraftTask_Operators.cpp"
    ).read_text(encoding="utf-8")
    method = "\n".join(
        extract(source, name)
        for name in (
            "scan_processing_operators",
            "update_processing_operator_mood",
            "select_processing_operator",
        )
    )
    (OUT / "main.cpp").write_text(
        FIXTURE.replace("// ACTUAL_METHODS", method), encoding="utf-8"
    )
    (OUT / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.20)\nproject(SkillSummarySelection LANGUAGES CXX)\n"
        f'add_executable(selection main.cpp "{ROOT.as_posix()}/src/MaaCore/Utils/ProcessingOperatorScore.cpp")\n'
        f'target_include_directories(selection PRIVATE "{ROOT.as_posix()}/src/MaaCore")\n'
        "target_compile_features(selection PRIVATE cxx_std_20)\n",
        encoding="utf-8",
    )
    subprocess.run(["cmake", "-S", str(OUT), "-B", str(OUT / "native")], check=True)
    subprocess.run(
        ["cmake", "--build", str(OUT / "native"), "--config", "Debug"], check=True
    )
    executable = next(
        p
        for p in [OUT / "native/Debug/selection.exe", OUT / "native/selection"]
        if p.exists()
    )
    subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
