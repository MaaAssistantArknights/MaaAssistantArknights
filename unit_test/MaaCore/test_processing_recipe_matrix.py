"""Cover all shipped processing recipes and replay reproducible mixed-material plans."""

import json
from pathlib import Path
import random
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "build/processing-recipe-matrix"
SEED = 20260916

FIXTURE = r"""
#include "Utils/MaterialCraftPlanner.h"
#include "Utils/ProcessingOperatorScore.h"
#include <iostream>
#include <stdexcept>
using namespace asst;
using namespace asst::infrast;
int main() {
    int checks=0;
    auto require=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Check "+std::to_string(checks));};
    const std::vector<MaterialFormula> formulas=FORMULAS;
    std::unordered_set<std::string> outputs;
    for(const auto& f:formulas)outputs.insert(f.item_id);
    MaterialInventory inventory{{"4001",10000000}};
    for(const auto& f:formulas)for(const auto& cost:f.costs)
        if(!outputs.contains(cost.item_id))inventory[cost.item_id]=100000;
    MaterialCraftPlanner planner(formulas);
    for(const auto& f:formulas) {
        const bool book=f.is_skill_summary();
        require(f.buff_type==(book?"W_SKILL":"W_EVOLVE"));
        require(f.ap_cost>0&&f.ap_cost%360000==0);
        const int original=f.ap_cost/360000;
        const auto nian=score_processing_operator({"bskill_ws_evolve4","bskill_ws_nian"},"char_2014_nian",f);
        const auto horn=score_processing_operator({"bskill_ws_alloyblock","bskill_ws_evolve_cost1"},"char_4039_horn",f);
        const auto summary=score_processing_operator({"bskill_ws_skill3"},"char_1043_leizi2",f);
        if(book) {
            require(!nian&&!horn&&summary&&summary->bonus_percent==80&&summary->mood_cost==original);
        } else {
            require(nian&&nian->bonus_percent==100&&nian->mood_cost==original+2&&!summary);
            if(f.item_id=="31024")require(horn&&horn->bonus_percent==100&&horn->mood_cost==3&&horn->better_than(*nian));
            else require(!horn||horn->bonus_percent==0);
        }
        const auto single=planner.build({{{f.item_id,1}},inventory});
        require(single.valid&&single.missing.empty());
        require(single.operations.back().formula.item_id==f.item_id);
        require(single.operations.back().formula.buff_type==f.buff_type);
    }
    const std::vector<std::vector<MaterialAmount>> combinations=COMBINATIONS;
    int operations=0;
    for(const auto& targets:combinations) {
        const auto plan=planner.build({targets,inventory});
        require(plan.valid&&plan.missing.empty());
        auto replay=inventory;
        MaterialInventory produced;
        int64_t gold=0,ap=0;
        for(const auto& op:plan.operations) {
            ++operations;
            require(op.batches>0&&op.formula.facility=="Processing");
            for(const auto& cost:op.formula.costs) {
                const int spent=cost.count*op.batches;
                require(replay[cost.item_id]>=spent);
                replay[cost.item_id]-=spent;
            }
            const int count=op.formula.count*op.batches;
            replay[op.formula.item_id]+=count;
            produced[op.formula.item_id]+=count;
            gold+=static_cast<int64_t>(op.formula.gold_cost)*op.batches;
            ap+=static_cast<int64_t>(op.formula.ap_cost)*op.batches;
            const auto score=score_processing_operator({"bskill_ws_p_kalts2"},"char_1052_kalts2",op.formula);
            require(score&&score->bonus_percent==80&&score->mood_cost==op.formula.ap_cost/360000);
        }
        replay["4001"]-=static_cast<int>(gold);
        require(replay==plan.inventory&&gold==plan.gold_cost&&ap==plan.ap_cost);
        MaterialInventory requested;
        for(const auto& target:targets)requested[target.item_id]+=target.count;
        for(const auto& [item,count]:requested)require(produced[item]>=count);
        require(inventory.at("4001")==10000000);
    }
    std::cout<<"Recipes "<<formulas.size()<<", mixed plans "<<combinations.size()
             <<", operations "<<operations<<", checks "<<checks<<" passed\n";
}
"""


def main():
    recipes = [
        recipe
        for recipe in json.loads(
            (ROOT / "resource/material_recipes.json").read_text(encoding="utf-8")
        ).values()
        if recipe.get("facility", "Processing") == "Processing"
    ]
    recipes.sort(key=lambda recipe: recipe["itemId"])
    literals = []
    for recipe in recipes:
        costs = ",".join(
            "{" + json.dumps(cost["id"]) + "," + str(cost["count"]) + "}"
            for cost in recipe["costs"]
        )
        literals.append(
            "{"
            + ",".join(
                json.dumps(value)
                for value in (
                    recipe["formulaId"],
                    recipe["itemId"],
                    recipe["count"],
                    recipe["goldCost"],
                    recipe["apCost"],
                )
            )
            + ",{"
            + costs
            + '},"Processing",'
            + json.dumps(recipe["buffType"])
            + "}"
        )
    generator = random.Random(SEED)
    scenarios = []
    for _ in range(200):
        targets = [
            (generator.choice(recipes)["itemId"], generator.randint(1, 4))
            for _ in range(generator.randint(2, 6))
        ]
        scenarios.append(
            "{"
            + ",".join(
                "{" + json.dumps(item) + "," + str(count) + "}"
                for item, count in targets
            )
            + "}"
        )
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "main.cpp").write_text(
        FIXTURE.replace("FORMULAS", "{" + ",\n".join(literals) + "}").replace(
            "COMBINATIONS", "{" + ",\n".join(scenarios) + "}"
        ),
        encoding="utf-8",
    )
    (OUT / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.20)\nproject(ProcessingRecipeMatrix LANGUAGES CXX)\n"
        f'add_executable(matrix main.cpp "{ROOT.as_posix()}/src/MaaCore/Utils/MaterialCraftPlanner.cpp" '
        f'"{ROOT.as_posix()}/src/MaaCore/Utils/ProcessingOperatorScore.cpp")\n'
        f'target_include_directories(matrix PRIVATE "{ROOT.as_posix()}/src/MaaCore")\n'
        "target_compile_features(matrix PRIVATE cxx_std_20)\n",
        encoding="utf-8",
    )
    subprocess.run(["cmake", "-S", str(OUT), "-B", str(OUT / "native")], check=True)
    subprocess.run(
        ["cmake", "--build", str(OUT / "native"), "--config", "Debug"], check=True
    )
    executable = next(
        path
        for path in (OUT / "native/Debug/matrix.exe", OUT / "native/matrix")
        if path.exists()
    )
    subprocess.run([str(executable)], check=True)
    print(f"Random seed: {SEED}")


if __name__ == "__main__":
    main()
