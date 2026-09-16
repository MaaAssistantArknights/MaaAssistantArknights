"""Compile the production processing scorer and exercise unlock, exclusion, and ranking boundaries."""

import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "build/processing-score-test"

FIXTURE = r"""
#include "Utils/ProcessingOperatorScore.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace asst::infrast;
int main() {
    int checks=0;
    auto require=[&](bool ok) { ++checks;if(!ok)throw std::runtime_error("Check "+std::to_string(checks)); };
    auto score=[](std::initializer_list<const char*> icons,std::string_view id="",std::string_view item="3303") {
        std::unordered_set<std::string> skills;for(auto icon:icons)skills.emplace(icon);
        return score_skill_summary(skills,id,item);
    };
    for(auto item:{"3302","3303"}) {
        for(const auto& [icon,bonus]:std::vector<std::pair<const char*,int>>{
            {"bskill_ws_p1",40},{"bskill_ws_p2",50},{"bskill_ws_p3",60},
            {"bskill_ws_p4",65},{"bskill_ws_p5",70},{"bskill_ws_p_kalts2",80},
            {"bskill_ws_skill1",70},{"bskill_ws_skill2",75},{"bskill_ws_skill3",80}}) {
            auto r=score({icon},"",item);require(r&&r->bonus_percent==bonus);
            require(std::abs(r->byproduct_probability()-(.1+.001*bonus))<1e-12);
        }
        // A name or elite-material icon must not manufacture book skills.
        require(!score({},"char_188_helage",item));
        require(!score({"bskill_ws_evolve4","bskill_ws_cost_nian"},"char_2014_nian",item));
        require(!score({"bskill_ws_asc2"},"",item));
        require(!score({"bskill_ws_bonus1"},"",item));
        require(!score({"bskill_ws_bonus2","bskill_ws_skill3"},"",item));
        for(auto id:{"char_4019_ncdeer","char_271_spikes","char_4072_ironmn","char_458_rfrost"})
            require(!score({"bskill_ws_p2"},id,item));
    }
    require(score({"bskill_ws_skill1"},"char_131_flameb")->bonus_percent==70);
    require(score({"bskill_ws_skill3"},"char_131_flameb")->bonus_percent==80);
    require(score({"bskill_ws_p5"},"char_1052_kalts2")->bonus_percent==70);
    require(score({"bskill_ws_p5","bskill_ws_p_kalts2"})->bonus_percent==80);
    require(score({"bskill_ws_skill1","bskill_ws_skill3"})->bonus_percent==80);
    for(auto icon:{"bskill_ws_skill_cost1","bskill_ws_skill_cost2","bskill_ws_all_cost2"}) {
        auto low=score({icon});require(low&&low->bonus_percent==0&&low->mood_cost==1);
        require(!score({icon},"","3302"));
        auto unlocked=score({icon,"bskill_ws_skill2"});require(unlocked->mood_cost==1);
        require(score({icon,"bskill_ws_skill2"},"","3302")->mood_cost==1);
        require(score({"bskill_ws_skill3"})->better_than(*unlocked));
        require(unlocked->better_than(*score({"bskill_ws_skill2"})));
    }
    require(score({"bskill_ws_p4","bskill_ws_all_cost1"})->mood_cost==2);
    require(score({"bskill_ws_p2","bskill_ws_cost_magallan"})->mood_cost==2);
    require(score({"bskill_ws_p2","bskill_ws_rub"})->bonus_percent==50);
    auto thorns=score({"bskill_ws_p2","bskill_ws_recovery"});
    require(thorns->bonus_percent==50&&thorns->mood_cost==2&&thorns->refunds_mood);
    require(!thorns->better_than(*score({"bskill_ws_p2"})));
    require(!score({"bskill_ws_lolxh","bskill_ws_cost_lolxh"}));
    auto cathy=score({"bskill_ws_p7"});require(cathy->bonus_percent==50&&cathy->probability_is_lower_bound);
    require(score_skill_summary({"bskill_ws_p7"},"","3303",true)->bonus_percent==60);
    require(!score_skill_summary({"bskill_ws_p7"},"","3303",false)->probability_is_lower_bound);
    require(!score({"bskill_ws_skill3"},"","30014"));
    require(!score({"bskill_ws_skill3"},"","3301"));
    require(!score({"unknown_skill"}));
    auto elite=[](std::initializer_list<const char*> icons,std::string_view item,int mood,std::string_view id="") {
        std::unordered_set<std::string> skills;for(auto icon:icons)skills.emplace(icon);
        asst::MaterialFormula f;f.item_id=item;f.ap_cost=mood*360000;f.buff_type="W_EVOLVE";
        return score_processing_operator(skills,id,f);
    };
    for(auto c:{1,2,4,8}) {
        for(const auto& [icon,bonus]:std::vector<std::pair<const char*,int>>{
          {"bskill_ws_p1",40},{"bskill_ws_p2",50},{"bskill_ws_p3",60},
          {"bskill_ws_p4",65},{"bskill_ws_p5",70},{"bskill_ws_p_kalts2",80},
          {"bskill_ws_evolve1",70},{"bskill_ws_evolve2",75},
          {"bskill_ws_evolve3",80},{"bskill_ws_evolve4",100}}) {
            auto r=elite({icon},"30014",c);require(r&&r->bonus_percent==bonus&&r->mood_cost==c);
        }
        auto nian=elite({"bskill_ws_evolve4","bskill_ws_nian"},"30014",c);
        require(nian->bonus_percent==100&&nian->mood_cost==c+2);
        require(elite({"bskill_ws_orirock","bskill_ws_constant"},"30014",c)->mood_cost==2);
        require(elite({"bskill_ws_evolve3","bskill_ws_constant2"},"30014",c)->mood_cost==4);
        require(elite({"bskill_ws_evolve3","bskill_ws_constant2","bskill_ws_constant3"},"30014",c)->mood_cost==3);
        auto rub=elite({"bskill_ws_p2","bskill_ws_rub"},"30014",c);
        require(rub->bonus_percent==(c==2?90:50));
        auto ju=elite({"bskill_ws_cost_ju"},"30014",c);
        require(ju->bonus_percent==60&&ju->mood_cost==c-(c>=8?2:0));
        auto ju2=elite({"bskill_ws_cost_ju","bskill_ws_cost_ju2"},"30014",c);
        require(ju2->bonus_percent==80&&ju2->mood_cost==c-(c>=4?2:0));
        auto blem=elite({"bskill_ws_cost_blemishine","bskill_ws_free"},"30014",c);
        require(blem->bonus_percent==40&&blem->mood_cost==c-(c>=8?4:0));
        for(auto icon:{"bskill_ws_cost","bskill_ws_cost_magallan"})
          require(elite({"bskill_ws_p2",icon},"30014",c)->mood_cost==c-(c>=4?2:0));
        require(elite({"bskill_ws_p4","bskill_ws_all_cost1"},"30014",c)->mood_cost==c-(c==4?1:0));
        require(elite({"bskill_ws_p4","bskill_ws_all_cost2"},"30014",c)->mood_cost==c-(c==2?1:0));
        require(elite({"bskill_ws_evolve2","bskill_ws_evolve_cost"},"30014",c)->mood_cost==c-(c==2?1:0));
        require(elite({"bskill_ws_evolve2","bskill_ws_evolve_cost1"},"30014",c)->mood_cost==c-(c==4?1:0));
        require(elite({"bskill_ws_evolve2","bskill_ws_evolve_cost2"},"30014",c)->mood_cost==c-(c==4?2:0));
        require(elite({"bskill_ws_evolve3","bskill_ws_evolve_cost3"},"30014",c)->mood_cost==c-(c==8?4:0));
        auto cat=elite({"bskill_ws_lolxh","bskill_ws_cost_lolxh"},"30014",c);
        require(c<4?!cat:cat&&cat->mood_cost==c/4&&cat->bonus_percent==(c==8?50:0));
    }
    struct Family { const char* icon;std::vector<const char*> items;int bonus; };
    for(const auto& family:std::vector<Family>{
        {"bskill_ws_orirock",{"30012","30013","30014"},90},
        {"bskill_ws_device",{"30062","30063","30064"},90},
        {"bskill_ws_polyester",{"30032","30033","30034"},90},
        {"bskill_ws_ketone",{"30052","30053","30054"},80},
        {"bskill_ws_crystalline",{"31034","30145"},80},
        {"bskill_ws_alloyblock",{"31024"},100}}) {
      for(auto item:family.items)require(elite({family.icon},item,4)->bonus_percent==family.bonus);
      require(!elite({family.icon},"30024",4));
    }
    for(auto item:{"30042","30043","30044"}) {
      require(elite({"bskill_ws_oriron"},item,4,"char_363_toddi")->bonus_percent==90);
      auto bough=elite({"bskill_ws_oriron","bskill_ws_p8"},item,2,"char_4207_branch");
      require(item==std::string("30043")?bough&&bough->bonus_percent==90&&bough->mood_cost==1:!bough);
      auto unknown=elite({"bskill_ws_oriron"},item,4);
      require(item==std::string("30043")?unknown&&unknown->bonus_percent==90:!unknown);
    }
    require(elite({"bskill_ws_crystalline","bskill_ws_evolve_cost4"},"31034",4)->mood_cost==3);
    require(elite({"bskill_ws_crystalline","bskill_ws_evolve_cost4"},"30145",8)->mood_cost==7);
    require(!elite({"bskill_ws_evolve_cost4"},"30135",8));
    require(elite({"bskill_ws_evolve2","bskill_ws_evolve3"},"30014",4)->bonus_percent==80);
    require(!elite({},"30014",4,"char_2014_nian"));
    require(!elite({"bskill_ws_constant"},"30014",1));
    require(elite({"bskill_ws_constant"},"30014",4)->mood_cost==2);
    require(!elite({"bskill_ws_skill3","bskill_ws_skill_cost1"},"30014",2));
    require(!elite({"bskill_ws_build3","bskill_ws_build_cost2"},"30014",4));
    require(!elite({"bskill_ws_asc2","bskill_ws_asc_cost1"},"30014",2));
    for(auto id:{"char_4019_ncdeer","char_271_spikes","char_4072_ironmn","char_458_rfrost"})
      require(!elite({"bskill_ws_p2"},"30014",4,id));
    for(auto icon:{"bskill_ws_bonus1","bskill_ws_bonus2","bskill_ws_frost","bskill_ws_evolve_dorm1",
                  "bskill_ws_evolve_dorm2","bskill_ws_evolve_dorm3","bskill_ws_cost&dorm"})
      require(!elite({icon,"bskill_ws_p2"},"30014",4));
    auto horn=elite({"bskill_ws_alloyblock","bskill_ws_evolve_cost1"},"31024",4);
    auto nian=elite({"bskill_ws_evolve4","bskill_ws_nian"},"31024",4);
    require(horn->better_than(*nian));
    require(nian->better_than(*elite({"bskill_ws_cost_ju2"},"31024",4)));
    auto thorns_elite=elite({"bskill_ws_p2","bskill_ws_recovery"},"30014",4);
    require(thorns_elite->refunds_mood&&thorns_elite->mood_cost==4&&thorns_elite->bonus_percent==50);
    require(elite({"bskill_ws_p7","bskill_ws_cost"},"30014",4)->probability_is_lower_bound);
    // Fixed byproduct type and LMD savings do not increase probability or change the mood tie-break.
    for(auto icon:{"bskill_ws_drop_orirock","bskill_ws_drop_oriron","bskill_ws_drop_polyester","bskill_ws_free"})
      require(elite({"bskill_ws_evolve3",icon},"30014",4)->bonus_percent==80);
    { asst::MaterialFormula f;f.item_id="30014";f.ap_cost=1440000;
      require(!score_processing_operator({"bskill_ws_evolve4"},"",f));
      for(auto type:{"W_ASC","W_BUILDING"}) {f.buff_type=type;require(!score_processing_operator({"bskill_ws_p2"},"",f));}
      f.buff_type="W_EVOLVE";f.ap_cost=1;require(!score_processing_operator({"bskill_ws_p2"},"",f));
      f.ap_cost=0;require(!score_processing_operator({"bskill_ws_p2"},"",f));
      f.ap_cost=1440000;f.facility="Mfg";require(!score_processing_operator({"bskill_ws_p2"},"",f)); }
    std::cout<<"Processing score: "<<checks<<" checks passed\n";
}
"""


def main():
    # The existing facility analyzer must have templates for every score-bearing icon.
    config = json.loads((ROOT / "resource/infrast.json").read_text(encoding="utf-8"))[
        "Processing"
    ]["skills"]
    source = (ROOT / "src/MaaCore/Utils/ProcessingOperatorScore.cpp").read_text(
        encoding="utf-8"
    )
    import re

    used = set(re.findall(r'"(bskill_ws_[a-z0-9_&]+)"', source))
    templates = {p.name for p in (ROOT / "resource/template").rglob("*.png")}
    assert used <= config.keys()
    assert all(config[icon]["template"] in templates for icon in used)
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "main.cpp").write_text(FIXTURE, encoding="utf-8")
    (OUT / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.20)\nproject(ProcessingOperatorScore LANGUAGES CXX)\n"
        f'add_executable(score main.cpp "{ROOT.as_posix()}/src/MaaCore/Utils/ProcessingOperatorScore.cpp")\n'
        f'target_include_directories(score PRIVATE "{ROOT.as_posix()}/src/MaaCore")\n'
        "target_compile_features(score PRIVATE cxx_std_20)\n",
        encoding="utf-8",
    )
    subprocess.run(["cmake", "-S", str(OUT), "-B", str(OUT / "native")], check=True)
    subprocess.run(
        ["cmake", "--build", str(OUT / "native"), "--config", "Debug"], check=True
    )
    executable = next(
        p for p in [OUT / "native/Debug/score.exe", OUT / "native/score"] if p.exists()
    )
    subprocess.run([str(executable)], check=True)
    print(f"Recognition resources: {len(used)} skill icons have configured templates")


if __name__ == "__main__":
    main()
