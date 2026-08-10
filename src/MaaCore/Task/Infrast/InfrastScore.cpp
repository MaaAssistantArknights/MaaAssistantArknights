#include "InfrastScore.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <limits>
#include <numeric>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace asst::infrast
{
namespace
{
using SkillCounts = std::unordered_map<std::string, int>;

struct CombinationScore
{
    double value = -1;
    std::unordered_set<std::string> only_need;
};

bool has_skill(const ScoreOper& oper, std::string_view skill)
{
    return oper.skills.contains(std::string(skill));
}

bool has_any_skill(const ScoreOper& oper, std::initializer_list<std::string_view> skills)
{
    return std::ranges::any_of(skills, [&](std::string_view skill) { return has_skill(oper, skill); });
}

bool is_operator(const ScoreOper& oper, std::initializer_list<std::string_view> ids)
{
    return !oper.operator_id.empty() &&
           std::ranges::any_of(ids, [&](std::string_view id) { return oper.operator_id == id; });
}

bool is_selected(const ScoreContext& context, std::string_view id)
{
    return context.selected_operator_ids.contains(std::string(id));
}

SkillCounts count_skills(const std::vector<const ScoreOper*>& opers)
{
    SkillCounts result;
    for (const auto* oper : opers) {
        for (const auto& skill : oper->skills) {
            ++result[skill];
        }
    }
    return result;
}

int skill_count(const SkillCounts& counts, std::string_view skill)
{
    const auto iter = counts.find(std::string(skill));
    return iter == counts.end() ? 0 : iter->second;
}

bool has_skill(const SkillCounts& counts, std::string_view skill)
{
    return skill_count(counts, skill) > 0;
}

double floor_step(double value, double step)
{
    return std::floor(value / step) * step;
}

CombinationScore score_trade(const std::vector<const ScoreOper*>& opers, const ScoreContext& context)
{
    const int max_storage = context.level == 1 ? 6 : context.level == 2 ? 8 : 10;
    double base = 0;
    int storage = 0;
    int gold = context.gold_station_num;
    const auto all = count_skills(opers);
    const bool is_money = context.product == "Money";
    CombinationScore result;

    // 输入组合，计算平均加成，与实际收益的差距：
    // 1. 只考虑 8 小时平均收益，非实际换班间隔；
    // 2. 心情阈值以下的干员不考虑，也忽略心情消耗；
    // 3. 无法稳定计算的概率效果忽略。
    for (const auto* oper : opers) {
        for (const auto& icon : oper->skills) {
            if (icon == "bskill_tra_spd3") {
                base += 0.35;
            }
            else if (icon == "bskill_tra_spd&formula1") {
                base += 0.34; // 近似两种产物
            }
            else if (icon == "bskill_tra_spd&meet1") {
                base += 0.4; // 近似满级会客室
                if (is_selected(context, "char_4186_tmoris")) {
                    base += 0.05;
                }
            }
            else if (icon == "bskill_tra_ord_spd_ext2") {
                base += 0.25;
                if (is_selected(context, "char_4186_tmoris")) {
                    base += 0.05;
                }
            }
            else if (icon == "bskill_tra_ord_spd_ext3") {
                base += 0.3;
                if (is_selected(context, "char_4186_tmoris")) {
                    base += 0.05;
                }
            }
            else if (icon == "bskill_tra_share3") {
                base += 0.2 * (context.level - 1);
            }
            else if (icon == "bskill_tra_spd&cost") {
                base += 0.3;
            }
            else if (icon == "bskill_tra_spd&limit7") {
                base += 0.3;
                storage += 1;
            }
            else if (icon == "bskill_tra_spd&limit6") {
                base += 0.25;
                storage += 1;
            }
            else if (icon == "bskill_tra_spd&limit5") {
                base += 0.2;
                storage += 4;
                if (is_selected(context, "char_206_gnosis")) {
                    base -= 0.15;
                    storage += 6;
                }
            }
            else if (icon == "bskill_tra_share2") {
                base += 0.1 * (context.level - 1);
            }
            else if (icon == "bskill_tra_spd_bd2") {
                base += 0.005 * context.dormitory_capacity;
                if (is_selected(context, "char_391_rosmon") || is_selected(context, "char_436_whispr")) {
                    base += 0.44;
                }
            }
            else if (icon == "bskill_tra_spd&limit4") {
                base += 0.15;
                storage += 4;
                if (is_selected(context, "char_206_gnosis")) {
                    base -= 0.15;
                    storage += 6;
                }
            }
            else if (icon == "bskill_tra_spd&limit3") {
                base += 0.15;
                storage += 2;
                if (is_selected(context, "char_206_gnosis")) {
                    base -= 0.15;
                    storage += 6;
                }
            }
            else if (icon == "bskill_tra_spd&limit2") {
                base += 0.1;
                storage += 4;
            }
            else if (icon == "bskill_tra_spd&limit1") {
                base += 0.1;
                storage += 2;
            }
            else if (icon == "bskill_tra_closure") {
                base += 0.1;
            }
            else if (icon == "bskill_tra_spd2") {
                base += 0.3;
            }
            else if (icon == "bskill_tra_spd1") {
                base += 0.2;
            }
            else if (icon == "bskill_tra_par&per1") {
                base += 0.25;
                if (is_selected(context, "char_4087_ines")) {
                    base += 0.05;
                }
            }
            else if (icon == "bskill_tra_par&per2") {
                base += 0.3;
                if (is_selected(context, "char_1035_wisdel")) {
                    storage += 2;
                }
                if (is_selected(context, "char_4087_ines")) {
                    base += 0.05;
                }
            }
            else if (icon == "trade_ord_spd&par1" || icon == "trade_ord_spd&par2") {
                base += 0.3; // 道格拉斯帮加成暂不计算
            }
            else if (icon == "bskill_tra_ord_spd_ext0") {
                base += 0.25;
                if (is_selected(context, "char_4145_ulpia")) {
                    base += 0.05;
                }
            }
            else if (icon == "bskill_tra_ord_spd_ext1") {
                base += 0.3;
                if (is_selected(context, "char_4145_ulpia")) {
                    base += 0.1;
                }
            }
            else if (icon == "bskill_tra_spd&limit_down1") {
                base += 0.2;
                storage -= 2;
                if (is_selected(context, "char_206_gnosis")) {
                    base -= 0.15;
                    storage += 6;
                }
            }
            else if (icon == "bskill_tra_spd&limit_down2") {
                base += 0.25;
                storage -= 6;
                if (is_selected(context, "char_206_gnosis")) {
                    base -= 0.15;
                    storage += 6;
                }
            }
            else if (icon == "bskill_tra_flow_gc2") {
                base += 0.05;
                gold += (gold / 2) * 2;
            }
            else if (icon == "bskill_tra_flow_gc1") {
                base += 0.05;
                gold += (gold / 4) * 2;
            }
            else if (icon == "bskill_tra_spd&limit_felyne") {
                base += 0.05;
                storage += 2;
                if (is_selected(context, "char_1029_yato2") && is_selected(context, "char_1030_noirc2")) {
                    base += 0.361;
                }
            }
            else if (icon == "bskill_tra_spd&dorm2") {
                base += 0.02 * context.dormitory_level_sum;
            }
            else if (icon == "bskill_tra_spd&dorm1") {
                base += 0.01 * context.dormitory_level_sum;
            }
            else if (icon == "bskill_tra_bd_n2") {
                base += 0.01 * context.dormitory_capacity;
                if (is_selected(context, "char_473_mberry")) {
                    base += 0.21;
                }
                if (is_selected(context, "char_2024_chyue")) {
                    base += 0.15;
                }
            }
            else if (icon == "bskill_tra_limit&cost") {
                storage += 5;
            }
            else if (icon == "bskill_tra_limit&trade2") {
                storage += context.level;
            }
            else if (icon == "bskill_tra_wt&cost2" && is_money) {
                base += 0.02; // 裁缝 B 单独使用效果很小，取近似值
            }
            else if (icon == "bskill_tra_wt&cost1" && is_money) {
                base += 0.01;
            }
            else if (icon == "bskill_tra_long2" && is_money) {
                base += 0.02; // 投资 B 单独使用效果很小，取近似值
            }
            else if (icon == "bskill_tra_long1" && is_money) {
                base += 0.01;
            }
        }
    }

    // 应用全局性技能效果。
    if (has_skill(all, "bskill_tra_share1")) {
        base += (context.level - 1) * 0.15;
    }

    // 拉普兰德与德克萨斯。
    const bool texas = has_skill(all, "bskill_tra_texas1") || has_skill(all, "bskill_tra_texas2");
    if (has_skill(all, "bskill_tra_lappland1")) {
        if (texas) {
            storage += 2;
            base += 0.65;
        }
        if (is_selected(context, "char_4186_tmoris")) {
            base += 0.05;
        }
    }
    else if (has_skill(all, "bskill_tra_lappland2")) {
        if (texas) {
            storage += 4;
            base += 0.65;
        }
        if (is_selected(context, "char_4186_tmoris")) {
            base += 0.05;
        }
    }

    // 贝洛内与伺夜。
    if (has_skill(all, "bskill_tra_spd&meet1") && has_skill(all, "bskill_tra_ord_spd_ext2")) {
        base += 0.05;
        storage += 2;
    }
    else if (has_skill(all, "bskill_tra_spd&meet1") && has_skill(all, "bskill_tra_ord_spd_ext3")) {
        base += 0.1;
        storage += 2;
    }

    if (const int count = skill_count(all, "bskill_tra_spd_variable22")) {
        base += std::min(0.35, floor_step(base, 0.05)) * count;
    }
    if (const int count = skill_count(all, "bskill_tra_flow_gs2")) {
        base += 0.05 + (gold / 2) * 0.15 * count;
    }
    if (const int count = skill_count(all, "bskill_tra_flow_gs1")) {
        base += 0.05 + (gold / 4) * 0.15 * count;
    }
    if (const int count = skill_count(all, "bskill_tra_flow_gs")) {
        base += gold * 0.05 * count;
    }
    if (has_skill(all, "bskill_trade_ord_spd_variable")) {
        base += storage * 0.04;
    }
    if (has_skill(all, "bskill_tra_limit2spd")) {
        base += std::min(1.0, std::floor(storage / 5.0) * 0.25);
    }

    if (has_skill(all, "bskill_tra_limit_count")) {
        base += std::max(1.0, max_storage + storage - std::floor(base / 0.1)) * 0.04;
    }
    else if (has_skill(all, "bskill_tra_limit_diff")) {
        // 孑精零近似：一天三换，并且只在换班时收单。
        base += (max_storage + storage) * 0.04 / (context.level + 1.12) * 4.034;
    }

    // 巫恋组合会覆盖普通加成，并只保留组合需要的干员。
    if (has_skill(all, "bskill_tra_vodfox") && is_money) {
        if (context.level == 1) {
            base = 0;
        }
        else if (context.level == 2) {
            base = 0.46;
        }
        else if (has_skill(all, "bskill_tra_wt&cost2") && has_skill(all, "bskill_tra_long2")) {
            result.only_need = { "bskill_tra_vodfox", "bskill_tra_wt&cost2", "bskill_tra_long2" };
            base = 1.7192;
        }
        else if (has_skill(all, "bskill_tra_wt&cost2") && has_skill(all, "bskill_tra_long1")) {
            result.only_need = { "bskill_tra_vodfox", "bskill_tra_wt&cost2", "bskill_tra_long1" };
            base = 1.3205;
        }
        else if (has_skill(all, "bskill_tra_wt&cost2")) {
            result.only_need = { "bskill_tra_vodfox", "bskill_tra_wt&cost2" };
            base = 0.9218;
        }
        else if (has_skill(all, "bskill_tra_long2")) {
            result.only_need = { "bskill_tra_vodfox", "bskill_tra_long2", "bskill_tra_wt&cost1" };
            base = 1.4734 + 0.001 * skill_count(all, "bskill_tra_wt&cost1");
        }
        else if (has_skill(all, "bskill_tra_long1")) {
            result.only_need = { "bskill_tra_vodfox", "bskill_tra_long1", "bskill_tra_wt&cost1" };
            base = 1.1927 + 0.001 * skill_count(all, "bskill_tra_wt&cost1");
        }
        else {
            result.only_need = { "bskill_tra_vodfox", "bskill_tra_wt&cost1" };
            base = 0.912 + 0.001 * skill_count(all, "bskill_tra_wt&cost1");
        }
    }

    // 但书禁用巫恋、可露希尔；可露希尔禁用巫恋、但书。
    if (!has_skill(all, "bskill_tra_vodfox") && !has_skill(all, "bskill_tra_closure") && is_money) {
        if (has_skill(all, "bskill_tra_against")) {
            base = (1 + base) * 1.276 - 1;
        }
        if (has_skill(all, "bskill_tra_against2")) {
            base = (1 + base) * 1.556 - 1;
        }
    }
    if (!has_skill(all, "bskill_tra_against") && !has_skill(all, "bskill_tra_against2") &&
        !has_skill(all, "bskill_tra_vodfox") && is_money && has_skill(all, "bskill_tra_closure")) {
        base = (1 + base) * 1.804 - 1;
    }

    if (has_skill(all, "bskill_tra_spd&wt1")) {
        base = -1; // 禁用尤里卡
    }
    result.value = base;
    return result;
}

CombinationScore score_mfg(const std::vector<const ScoreOper*>& opers, const ScoreContext& context)
{
    const int max_storage = context.level == 1 ? 24 : context.level == 2 ? 36 : 54;
    const bool records = context.product == "CombatRecord";
    const bool gold_product = context.product == "PureGold";
    const bool origin_stone = context.product == "OriginStone";
    double base = 0;
    double station = 0;
    int robot = 0;
    int standard = 0;
    int rhine = 0;
    int metallics = 0;
    int abyssal_hunter = 0;
    bool station_only = false;
    std::vector<int> storage(opers.size(), 0);
    const auto all = count_skills(opers);
    CombinationScore result;

    // 四名候选是否齐全已在枚举组合前按完整干员列表检查。单个制造站最多只有三个位置，
    // 这里不能再要求四人同时出现在当前组合中，否则第一间制造站永远无法启用该组合。
    const bool abyssal_roster_complete = context.use_abyssal_hunter && is_selected(context, "char_474_glady");

    for (size_t index = 0; index < opers.size(); ++index) {
        const auto& oper = *opers[index];
        if (abyssal_roster_complete &&
            is_operator(oper, { "char_263_skadi", "char_143_ghost", "char_4145_ulpia", "char_218_cuttle" })) {
            ++abyssal_hunter;
            if (abyssal_hunter < 3 && !is_selected(context, oper.operator_id)) {
                base += 0.401;
            }
        }

        for (const auto& icon : oper.skills) {
            if (icon == "bskill_man_limit&cost5") {
                storage[index] += 10;
            }
            else if (icon == "bskill_man_exp3" && records) {
                base += 0.35;
            }
            else if (icon == "bskill_man_exp2" && records) {
                base += 0.3001;
            }
            else if ((icon == "bskill_formula_spd_headb1" || icon == "bskill_formula_spd_headb2") && records) {
                base += 0.3;
            }
            else if (icon == "bskill_man_exp1" && records) {
                base += 0.2501;
            }
            else if (icon == "bskill_man_exp4" && records) {
                base += 0.2;
            }
            else if (icon == "bskill_man_exp&spd&limit&cost2" && records) {
                base += 0.35;
            }
            else if (icon == "bskill_man_exp&spd&limit&cost1" && records) {
                base += 0.2;
            }
            else if (icon == "bskill_man_exp&spd&cost1" && records) {
                base += 0.34999;
            }
            else if (icon == "bskill_man_gold2") {
                if (gold_product) {
                    base += 0.35;
                    ++metallics;
                }
                if (is_selected(context, "char_4098_vvana")) {
                    base += 0.07;
                }
            }
            else if (icon == "bskill_man_gold1" && gold_product) {
                base += 0.3001;
                ++metallics;
            }
            else if (icon == "bskill_man_spd&dorm1" && gold_product) {
                base += 0.01 * context.dormitory_level_sum;
            }
            else if (icon == "bskill_man_spd&trade" && gold_product) {
                station += 0.2 * context.trading_station_num;
            }
            else if (icon == "bskill_man_spd&trade1" && gold_product) {
                base += 0.03 * context.trading_station_num;
            }
            else if (icon == "bskill_man_gold3") {
                if (gold_product) {
                    base += 0.25;
                }
                if (is_selected(context, "char_1034_jesca2")) {
                    base += 0.05;
                }
            }
            else if (icon == "bskill_man_gold&blacksteel" && gold_product) {
                base += 0.02;
                if (is_selected(context, "char_1034_jesca2")) {
                    base += 0.02;
                }
            }
            else if (icon == "bskill_man_spd_bd2") {
                if (is_selected(context, "char_436_whispr")) {
                    base += 0.40999;
                }
            }
            else if (icon == "bskill_man_spd_bd6") {
                if (is_selected(context, "char_473_mberry")) {
                    base += 0.4 / 2.5;
                }
                if (is_selected(context, "char_2024_chyue")) {
                    base += 0.05 / 2.5;
                    if (is_selected(context, "char_2023_ling")) {
                        base += 0.05 / 2.5;
                    }
                    if (is_selected(context, "char_2015_dusk")) {
                        base += 0.05 / 2.5;
                    }
                }
                if (is_selected(context, "char_2023_ling")) {
                    base += 0.15 / 2.5;
                }
            }
            else if (icon == "bskill_man_spd_bd7") {
                if (is_selected(context, "char_473_mberry")) {
                    base += 0.4 / 3;
                }
                if (is_selected(context, "char_2024_chyue")) {
                    base += 0.1 / 3;
                    if (is_selected(context, "char_2023_ling")) {
                        base += 0.05 / 3;
                    }
                    if (is_selected(context, "char_2015_dusk")) {
                        base += 0.05 / 3;
                    }
                }
                if (is_selected(context, "char_2023_ling")) {
                    base += 0.15 / 3;
                }
            }
            else if (icon == "bskill_man_spd3") {
                base += 0.3;
                if (is_operator(oper, { "char_1031_slent2" })) {
                    ++rhine;
                }
            }
            else if (icon == "bskill_man_spd2") {
                base += 0.25;
                if (is_operator(
                        oper,
                        { "char_210_stward",
                          "char_240_wyvern",
                          "char_181_flower",
                          "char_235_jesica",
                          "char_437_mizuki",
                          "char_484_robrta",
                          "char_4066_highmo" })) {
                    ++standard;
                }
                else if (
                    is_operator(oper, { "char_128_plosis", "char_108_silent", "char_4048_doroth", "char_135_halo" })) {
                    ++rhine;
                }
                else if (is_operator(oper, { "char_430_fartth", "char_431_ashlok", "char_496_wildmn" })) {
                    if (is_selected(context, "char_4098_vvana")) {
                        base += 0.07;
                    }
                    if (is_selected(context, "char_420_flamtl")) {
                        base += records ? 0.101 : gold_product ? -0.1 : 0;
                    }
                    if (is_selected(context, "char_4000_jnight") && is_operator(oper, { "char_496_wildmn" })) {
                        base += 0.052;
                    }
                }
                if (is_selected(context, "char_1034_jesca2") &&
                    is_operator(oper, { "char_240_wyvern", "char_235_jesica" })) {
                    base += 0.05;
                }
            }
            else if (icon == "bskill_man_limit&cost3") {
                storage[index] += 16;
            }
            else if (icon == "bskill_man_spd&limit&cost3") {
                base += 0.25;
                storage[index] -= 12;
            }
            else if (icon == "bskill_man_spd&limit6") {
                base += 0.2;
                storage[index] -= 8;
            }
            else if (icon == "bskill_man_spd_add1") {
                base += 0.23125;
            }
            else if (icon == "bskill_man_spd_veen") {
                base += 0.299; // 认为训练室三级
            }
            else if (icon == "bskill_man_spd_reduce") {
                base += 0.22;
            }
            else if (icon == "bskill_man_spd_add2") {
                base += 0.2125;
            }
            else if (icon == "bskill_man_marcille1") {
                base += 0.2;
            }
            else if (icon == "bskill_man_marcille2") {
                base += 0.3;
            }
            else if (icon == "bskill_man_spd&cost1" || icon == "bskill_man_spd&cost2") {
                base += 0.2;
            }
            else if (icon == "bskill_man_spd&cost3") {
                base += 0.3;
            }
            else if (icon == "bskill_man_spd&limit5") {
                base += 0.25;
                storage[index] += 6;
            }
            else if (icon == "bskill_man_spd_add3") {
                base += 0.1; // 十小时平均收益，八小时收益约 0.073
            }
            else if (icon == "bskill_man_spd1") {
                base += 0.15;
                if (is_operator(
                        oper,
                        { "char_502_nblade",
                          "char_126_shotst",
                          "char_452_bstalk",
                          "char_4041_chnut",
                          "char_4063_quartz",
                          "char_464_cement",
                          "char_1036_fang2" })) {
                    ++standard;
                }
                else if (is_operator(oper, { "char_128_plosis", "char_108_silent", "char_135_halo" })) {
                    ++rhine;
                }
                else if (is_operator(oper, { "char_430_fartth", "char_431_ashlok", "char_496_wildmn" })) {
                    if (is_selected(context, "char_4098_vvana")) {
                        base += 0.07;
                    }
                    if (is_selected(context, "char_420_flamtl")) {
                        base += records ? 0.101 : gold_product ? -0.1 : 0;
                    }
                    if (is_selected(context, "char_4000_jnight") && is_operator(oper, { "char_496_wildmn" })) {
                        base += 0.052;
                    }
                }
            }
            else if (icon == "bskill_man_spd&limit3") {
                base += 0.1;
                storage[index] += 10;
            }
            else if (icon == "bskill_man_spd&limit1") {
                base += 0.1;
                storage[index] += 6;
            }
            else if (icon == "bskill_man_spd&limit&cost2") {
                base -= 0.05;
                storage[index] += 19;
            }
            else if (icon == "bskill_man_spd&limit&cost1") {
                base -= 0.05;
                storage[index] += 16;
            }
            else if (icon == "bskill_man_spd&limit&cost4") {
                base -= 0.2;
                storage[index] += 17;
            }
            else if (icon == "bskill_man_fuze") {
                base += 0.2;
            }
            else if (icon == "bskill_man_gold4" && gold_product) {
                base += 0.25;
            }
            else if (icon == "bskill_man_exp&limit2" && records) {
                storage[index] += 15;
            }
            else if (icon == "bskill_man_exp&limit1" && records) {
                storage[index] += 12;
            }
            else if (icon == "bskill_man_exp&limit3" && records) {
                base -= 0.0001;
                storage[index] += 4;
            }
            else if (icon == "bskill_man_limit&cost2") {
                storage[index] += 10;
            }
            else if (icon == "bskill_man_limit&cost1") {
                storage[index] += 8;
            }
            else if (icon == "bskill_man_originium2" && origin_stone) {
                base += 0.35;
            }
            else if (icon == "bskill_man_originium1" && origin_stone) {
                base += 0.3001;
            }
            else if (icon == "bskill_man_constrlv") {
                robot = std::min(64, robot + context.total_station_level);
            }
        }
    }

    if (has_skill(all, "bskill_man_spd_bd3")) {
        base += (robot / 16) * 0.05;
    }
    if (has_skill(all, "bskill_man_spd_bd4")) {
        base += (robot / 8) * 0.05;
    }

    const bool tragodia =
        has_skill(all, "bskill_man_exp&spd&limit&cost2") || has_skill(all, "bskill_man_exp&spd&limit&cost1");
    if (has_skill(all, "bskill_man_spd_double2") && tragodia && records) {
        base += 0.3;
    }

    if (const int bubble_count = skill_count(all, "bskill_man_spd_variable31")) {
        // 泡泡按每名干员的仓储效果分别结算。
        for (const int value : storage) {
            if (value > 0 && value <= 16) {
                base += value * 0.01 * bubble_count;
            }
            else if (value > 16) {
                base += value * 0.03 * bubble_count;
            }
        }
    }
    else if (const int vermeil_count = skill_count(all, "bskill_man_spd_variable11")) {
        const int sum = std::max(0, std::accumulate(storage.begin(), storage.end(), 0));
        base += sum * 0.02 * vermeil_count;
    }
    if (const int count = skill_count(all, "bskill_man_spd_variable21")) {
        base += std::min(0.4, floor_step(base, 0.05) - abyssal_hunter * 0.4) * count;
    }

    // 发电站数量类技能只与同类设施加成组合，白板位置不强制占用。
    if (const int count = skill_count(all, "bskill_man_spd&power3")) {
        station_only = true;
        station += 0.16 * context.virtual_power_station_num * count;
        result.only_need.emplace("bskill_man_spd&power3");
    }
    if (const int count = skill_count(all, "bskill_man_spd&power2")) {
        station_only = true;
        station += 0.11 * context.virtual_power_station_num * count;
        result.only_need.emplace("bskill_man_spd&power2");
    }
    if (const int count = skill_count(all, "bskill_man_spd&power1")) {
        station_only = true;
        station += 0.06 * context.virtual_power_station_num * count;
        result.only_need.emplace("bskill_man_spd&power1");
    }
    if (has_skill(all, "bskill_man_spd_manu2")) {
        station_only = true;
        station += 0.1 * context.level;
        result.only_need.emplace("bskill_man_spd_manu2");
    }
    if (const int count = skill_count(all, "bskill_man_skill_spd")) {
        base += standard * 0.0501 * count - 0.0002;
    }
    if (const int count = skill_count(all, "bskill_man_skill_spd2")) {
        base += rhine * 0.0501 * count - 0.0002;
    }
    if (const int count = skill_count(all, "bskill_man_gold&rhine"); count && gold_product) {
        base += rhine * 0.03 * count;
    }
    if (const int count = skill_count(all, "bskill_man_skill_spd3"); count && gold_product) {
        base += metallics * 0.0501 * count - 0.0002;
    }

    const int storage_sum = std::accumulate(storage.begin(), storage.end(), 0);
    if (max_storage + storage_sum < 20 || storage_sum < -15) {
        base = -1;
    }
    if (station_only) {
        base = station;
        result.only_need.emplace("bskill_man_spd&trade");
    }
    else {
        base += station;
    }
    result.value = base;
    return result;
}

std::vector<size_t> eligible_indices(const std::vector<ScoreOper>& opers, const ScoreContext& context)
{
    std::vector<size_t> result;
    for (size_t index = 0; index < opers.size(); ++index) {
        if (opers[index].mood_ratio >= context.mood_threshold) {
            result.emplace_back(index);
        }
    }
    return result;
}

ScoreResult select_combinations(const std::vector<ScoreOper>& opers, const ScoreContext& context)
{
    const auto eligible = eligible_indices(opers, context);
    const size_t count = std::min(eligible.size(), static_cast<size_t>(std::max(0, context.slots)));
    ScoreResult best;
    if (count == 0) {
        best.score = 0;
        return best;
    }

    std::vector<size_t> current;
    std::unordered_set<std::string> best_only_need;
    std::function<void(size_t)> visit = [&](size_t begin) {
        if (current.size() == count) {
            std::vector<const ScoreOper*> selected;
            selected.reserve(current.size());
            for (const size_t index : current) {
                selected.emplace_back(&opers[index]);
            }
            const CombinationScore score =
                context.facility == "Trade" ? score_trade(selected, context) : score_mfg(selected, context);
            if (score.value > best.score || best.indices.empty()) {
                best.indices = current;
                best.score = score.value;
                best_only_need = score.only_need;
            }
            return;
        }

        const size_t missing = count - current.size();
        for (size_t pos = begin; pos + missing <= eligible.size(); ++pos) {
            current.emplace_back(eligible[pos]);
            visit(pos + 1);
            current.pop_back();
        }
    };
    visit(0);

    // 特殊组合只需要部分干员，其他位置留给后续房间使用。
    if (!best_only_need.empty()) {
        std::erase_if(best.indices, [&](size_t index) {
            return std::ranges::none_of(opers[index].skills, [&](const std::string& skill) {
                return best_only_need.contains(skill);
            });
        });
    }
    return best;
}

double office_score(const ScoreOper& oper)
{
    double score = 0;
    for (const auto& icon : oper.skills) {
        if (icon == "bskill_hire_skgoat2") {
            score += 0.5;
        }
        else if (icon == "bskill_hire_skgoat3") {
            score += 0.3;
        }
        else if (icon == "bskill_hire_skgoat4" || icon == "bskill_hire_skgoat") {
            score += 0.45;
        }
        else if (icon == "bskill_hire_spd5") {
            score += 0.45;
        }
        else if (icon == "bskill_hire_spd4") {
            score += 0.4;
        }
        else if (icon == "bskill_hire_spd3") {
            score += 0.35;
        }
        else if (icon == "bskill_hire_spd&clue") {
            score += 0.36;
        }
        else if (icon == "bskill_hire_spd2" || icon == "bskill_ws_p_kalts2") {
            score += 0.3;
        }
        else if (icon == "bskill_hire_spd&ursus2") {
            score += 0.2;
        }
        else if (icon == "bskill_hire_spd_memento") {
            score += 0.31;
        }
        else if (icon == "bskill_hire_spd_bd_n2") {
            score += 0.301;
        }
        else if (
            icon == "bskill_hire_spd&blacksteel2" || icon == "bskill_hire_spd_bd_n1_n1" ||
            icon == "bskill_hire_spd&cost2" || icon == "bskill_hire_blitz" || icon == "bskill_hire_spd1") {
            score += 0.2;
        }
        else if (icon == "bskill_hire_spd&cost1" || icon == "bskill_hire_spd") {
            score += 0.1;
        }
    }
    return score;
}

double power_score(const ScoreOper& oper, const ScoreContext& context)
{
    double score = 0;
    const bool robot = is_operator(
        oper,
        { "char_285_medic2",
          "char_286_cast3",
          "char_376_therex",
          "char_4000_jnight",
          "char_4093_frston",
          "char_4136_phonor" });
    for (const auto& icon : oper.skills) {
        if (icon == "bskill_pow_count") {
            score += (is_selected(context, "char_4000_jnight") || context.workbench_num > 0) ? -1 : 0.1;
        }
        else if (icon == "bskill_pow_spd1") {
            score += 0.1;
            if (robot && is_selected(context, "char_1027_greyy2")) {
                score -= 1;
            }
            if (is_operator(oper, { "char_285_medic2" }) && is_selected(context, "char_4000_jnight") &&
                !is_selected(context, "char_1027_greyy2")) {
                score += 0.101;
            }
        }
        else if (icon == "bskill_pow_spd_p1") {
            if (is_selected(context, "char_1027_greyy2")) {
                score -= 1;
            }
        }
        else if (icon == "bskill_power_rec_spd&addition2") {
            score += 0.15;
        }
        else if (icon == "bskill_power_rec_spd&addition1") {
            score += 0.13;
        }
        else if (icon == "bskill_power_rec_rhine") {
            score += 0.11;
        }
        else if (icon == "bskill_pow_drone") {
            score += 0.22;
        }
        else if (icon == "bskill_pow_spd3") {
            score += 0.2;
            if (robot) {
                score -= 0.2; // 防止作业平台的相似图标误判
            }
        }
        else if (icon == "bskill_pow_spd2") {
            score += 0.15;
            if (is_operator(oper, { "char_472_pasngr", "char_385_finlpp" })) {
                score -= 1;
            }
        }
        else if (icon == "bskill_pow_spd&cost") {
            if (is_selected(context, "char_003_kalts")) {
                score += 0.05;
            }
            if (is_selected(context, "char_1027_greyy2")) {
                score -= 1;
            }
        }
        else if (icon == "bskill_pow_jnight") {
            if (is_selected(context, "char_1027_greyy2")) {
                score -= 1;
            }
            else if (is_selected(context, "char_420_flamtl") || is_selected(context, "char_4098_vvana")) {
                score += 0.101;
            }
        }
    }
    return score;
}

ScoreResult select_single(const std::vector<ScoreOper>& opers, const ScoreContext& context)
{
    ScoreResult result;
    for (const size_t index : eligible_indices(opers, context)) {
        const double score =
            context.facility == "Office" ? office_score(opers[index]) : power_score(opers[index], context);
        if (result.indices.empty() || score > result.score) {
            result.indices = { index };
            result.score = score;
        }
    }
    return result;
}

ScoreResult select_reception(const std::vector<ScoreOper>& opers, const ScoreContext& context)
{
    std::vector<size_t> priority;
    std::vector<size_t> preferred;
    std::vector<size_t> remain;
    for (const size_t index : eligible_indices(opers, context)) {
        const auto& oper = opers[index];
        if (has_skill(oper, "bskill_meet_spdowned1")) {
            continue; // 禁用尤里卡
        }
        if (has_any_skill(oper, { "bskill_meet_spd&cost", "bskill_meet_exchange" }) ||
            (has_skill(oper, "bskill_meet_spd_confes1") && is_selected(context, "char_300_phenxi"))) {
            priority.emplace_back(index);
        }
        else if (
            has_any_skill(oper, { "bskill_meet_spdnotowned2", "bskill_meet_spd_hast1" }) ||
            (has_skill(oper, "bskill_meet_spd3") && !is_operator(oper, { "char_427_vigil" }))) {
            preferred.emplace_back(index);
        }
        else {
            remain.emplace_back(index);
        }
    }
    priority.insert(priority.end(), preferred.begin(), preferred.end());
    priority.insert(priority.end(), remain.begin(), remain.end());
    if (priority.size() > static_cast<size_t>(std::max(0, context.slots))) {
        priority.resize(context.slots);
    }
    return { std::move(priority), 0 };
}

ScoreResult select_control(const std::vector<ScoreOper>& opers, const ScoreContext& context)
{
    const auto eligible = eligible_indices(opers, context);
    std::vector<size_t> best;
    bool manu_acc = false;
    bool trading_acc = false;
    bool mood_reduce = false;
    bool office_acc = false;
    const size_t limit = static_cast<size_t>(std::max(0, context.slots));

    auto contains = [&](size_t index) {
        return std::ranges::find(best, index) != best.end();
    };
    auto add_first = [&](const auto& predicate) {
        if (best.size() >= limit) {
            return false;
        }
        const auto iter =
            std::ranges::find_if(eligible, [&](size_t index) { return !contains(index) && predicate(opers[index]); });
        if (iter == eligible.end()) {
            return false;
        }
        best.emplace_back(*iter);
        return true;
    };

    // 诗怀雅与龙门近卫局制造加速必须同时存在，否则整组放弃。
    if (limit >= 2) {
        const auto swire = std::ranges::find_if(eligible, [&](size_t index) {
            return is_operator(opers[index], { "char_308_swire" });
        });
        const auto guard = std::ranges::find_if(eligible, [&](size_t index) {
            return has_skill(opers[index], "bskill_token_prod_spd3_lungmenguard");
        });
        if (swire != eligible.end() && guard != eligible.end() && *swire != *guard) {
            best = { *swire, *guard };
            trading_acc = true;
            manu_acc = true;
        }
    }

    // 麒麟夜刀与火龙黑角组合。
    if (!manu_acc && !trading_acc && limit >= 2) {
        const auto yato = std::ranges::find_if(eligible, [&](size_t index) {
            return has_skill(opers[index], "bskill_ctrl_token_p_spd2") &&
                   has_skill(opers[index], "bskill_ctrl_cost_felyne");
        });
        const auto noir = std::ranges::find_if(eligible, [&](size_t index) {
            return has_any_skill(opers[index], { "bskill_ctrl_token_t_spd", "bskill_ctrl_felyne" });
        });
        if (yato != eligible.end() && noir != eligible.end() && *yato != *noir) {
            best = { *yato, *noir };
            manu_acc = true;
            trading_acc = has_skill(opers[*noir], "bskill_ctrl_token_t_spd");
        }
    }

    if (!trading_acc && add_first([](const ScoreOper& oper) {
            return has_any_skill(oper, { "bskill_ctrl_t_spd", "bskill_ctrl_tra&prod" });
        })) {
        trading_acc = true;
    }

    add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_psk"); });
    add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_fraction_knight"); });

    if (is_selected(context, "char_436_whispr")) {
        add_first([](const ScoreOper& oper) {
            return has_skill(oper, "bskill_ctrl_cost_bd1") && has_skill(oper, "bskill_ctrl_cost_bd2") &&
                   oper.mood_ratio > 22.0 / 24.0;
        });
    }
    if (is_selected(context, "char_473_mberry")) {
        add_first([](const ScoreOper& oper) {
            return has_skill(oper, "bskill_ctrl_cost_bd1&bd2") && oper.mood_ratio > 22.0 / 24.0;
        });
    }

    // 深海队只有在选项开启且不会挤掉完整发电/骑士联动时使用，高心情是必要条件。
    if (context.use_abyssal_hunter &&
        !(is_selected(context, "char_1027_greyy2") && is_selected(context, "char_420_flamtl") &&
          is_selected(context, "char_4098_vvana"))) {
        add_first([](const ScoreOper& oper) {
            return has_skill(oper, "bskill_ctrl_aegir2") && oper.mood_ratio > 22.0 / 24.0;
        });
    }

    if ((is_selected(context, "char_436_whispr") || is_selected(context, "char_473_mberry")) &&
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_h_spd"); })) {
        office_acc = true;
    }
    if (!office_acc && add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_hire_tmoris"); })) {
        office_acc = true;
    }
    if (!manu_acc && add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_p_spd"); })) {
        manu_acc = true;
    }

    if (is_selected(context, "char_473_mberry") &&
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_cost_bd3"); })) {
        mood_reduce = true;
    }
    if (std::ranges::any_of(best, [&](size_t index) { return is_operator(opers[index], { "char_2024_chyue" }); })) {
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_cost_bd1"); });
    }

    // 丰川祥子技能可与制造加速叠加，后续同团成员按固定顺序补入。
    const bool oblivionis = add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_p_oblvns"); });
    if (oblivionis || is_selected(context, "char_4182_oblvns")) {
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_trade_mortis"); });
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_dorm_uika1"); });
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_meet_amoris1"); });
    }

    if (!manu_acc && context.workbench_num > 1 &&
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_token_p_spd"); })) {
        manu_acc = true;
    }
    if (!is_selected(context, "char_436_whispr") && !is_selected(context, "char_473_mberry")) {
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_t_limit&spd"); });
    }

    if (!mood_reduce) {
        if (add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_cost_bd4"); }) ||
            add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_lonely"); }) ||
            add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_cost_expand"); })) {
            mood_reduce = true;
        }
    }

    if (is_selected(context, "char_285_medic2")) {
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_p_bot"); });
    }
    add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_bd_spd"); });
    if (is_selected(context, "char_1035_wisdel")) {
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_c_spd1"); });
    }
    if (!std::ranges::any_of(best, [&](size_t index) { return is_operator(opers[index], { "char_474_glady" }); })) {
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_ela"); });
    }

    const bool mlynar =
        std::ranges::any_of(best, [&](size_t index) { return is_operator(opers[index], { "char_4064_mlynar" }); });
    if (mlynar) {
        while (add_first([](const ScoreOper& oper) {
            return has_skill(oper, "bskill_ctrl_cost") && !has_skill(oper, "bskill_ctrl_p_spd");
        })) {
        }
    }
    while (add_first(
        [](const ScoreOper& oper) { return has_any_skill(oper, { "bskill_ctrl_sp", "bskill_ctrl_cost" }); })) {
    }

    const double score = static_cast<double>(best.size());
    return { std::move(best), score };
}

ScoreResult select_dorm(const std::vector<ScoreOper>& opers, const ScoreContext& context)
{
    const auto eligible = eligible_indices(opers, context);
    std::vector<size_t> result;
    const size_t limit = static_cast<size_t>(std::max(0, context.slots));

    // 感知信息与人间烟火联动时，优先选择对应的全体恢复宿管。
    if (is_selected(context, "char_391_rosmon") || is_selected(context, "char_436_whispr")) {
        for (const size_t index : eligible) {
            const auto& oper = opers[index];
            if ((has_skill(oper, "bskill_dorm_all&bd_n1_2") && has_skill(oper, "bskill_dorm_all&bd_n1_n3")) ||
                (has_skill(oper, "bskill_dorm_all&bd_n1") && has_skill(oper, "bskill_dorm_all&bd_n1_n2")) ||
                has_any_skill(oper, { "bskill_dorm_bdnum", "bskill_dorm_rec_allbd" }) ||
                is_operator(oper, { "char_245_cello" })) {
                result.emplace_back(index);
                if (result.size() >= limit) {
                    break;
                }
            }
        }
        return { std::move(result), 0 };
    }

    if (is_selected(context, "char_4055_bgsnow")) {
        for (const size_t index : eligible) {
            const auto& oper = opers[index];
            if (has_any_skill(oper, { "bskill_dorm_all&one1", "bskill_dorm_all&one2" }) ||
                (has_skill(oper, "bskill_dorm_all2") && is_operator(oper, { "char_151_myrtle" }))) {
                result.emplace_back(index);
                if (result.size() >= limit) {
                    break;
                }
            }
        }
        return { std::move(result), 0 };
    }

    auto score_all = [&](const ScoreOper& oper) {
        double score = 0;
        for (const auto& icon : oper.skills) {
            if (icon == "bskill_dorm_all1") {
                score += 0.1;
            }
            else if (icon == "bskill_dorm_all2") {
                score += 0.15;
            }
            else if (icon == "bskill_dorm_all3") {
                score += 0.2;
            }
            else if (icon == "bskill_dorm_all&one1") {
                score += 0.2;
            }
            else if (icon == "bskill_dorm_all&one2") {
                score += 0.25;
            }
            else if (icon == "bskill_dorm_all&one3") {
                score += 0.1;
            }
            else if (icon == "bskill_hire_spd3") {
                score += 0.35;
            }
            else if (icon == "bskill_dorm_powtorecall2") {
                score += 0.15 + context.virtual_power_station_num * 0.05;
            }
        }
        return score;
    };
    auto score_single = [](const ScoreOper& oper) {
        double score = 0;
        for (const auto& icon : oper.skills) {
            if (icon == "bskill_dorm_single1") {
                score += 0.55;
            }
            else if (icon == "bskill_dorm_single2") {
                score += 0.65;
            }
            else if (icon == "bskill_dorm_single3") {
                score += 0.7;
            }
            else if (icon == "bskill_dorm_single4") {
                score += 0.75;
            }
            else if (icon == "bskill_dorm_single&one01") {
                score += 0.2;
            }
            else if (icon == "bskill_dorm_single&one02") {
                score += 0.25;
            }
            else if (icon == "bskill_dorm_single&one21") {
                score += 0.4;
            }
            else if (icon == "bskill_dorm_single&one22") {
                score += 0.5;
            }
            else if (icon == "bskill_dorm_single&one11") {
                score += 0.3;
            }
            else if (icon == "bskill_dorm_single&one12") {
                score += 0.35;
            }
        }
        return score;
    };

    auto best_for = [&](const auto& scorer, std::optional<size_t> excluded = std::nullopt) -> std::optional<size_t> {
        std::optional<size_t> best;
        double best_score = -std::numeric_limits<double>::infinity();
        for (const size_t index : eligible) {
            if (excluded && index == *excluded) {
                continue;
            }
            const double score = scorer(opers[index]);
            if (!best || score > best_score) {
                best = index;
                best_score = score;
            }
        }
        return best;
    };

    if (const auto all = best_for(score_all)) {
        result.emplace_back(*all);
    }
    if (result.size() < limit) {
        if (const auto single = best_for(score_single, result.empty() ? std::nullopt : std::optional(result.front()))) {
            result.emplace_back(*single);
        }
    }
    return { std::move(result), 0 };
}
} // namespace

ScoreResult select_best_opers(const std::vector<ScoreOper>& opers, const ScoreContext& context)
{
    if (context.facility == "Mfg" || context.facility == "Trade") {
        return select_combinations(opers, context);
    }
    if (context.facility == "Office" || context.facility == "Power") {
        return select_single(opers, context);
    }
    if (context.facility == "Reception") {
        return select_reception(opers, context);
    }
    if (context.facility == "Control") {
        return select_control(opers, context);
    }
    if (context.facility == "Dorm") {
        return select_dorm(opers, context);
    }
    return { };
}

bool should_short_circuit_mfg(
    bool enabled,
    bool use_abyssal_hunter,
    int occupied_slots,
    int room_slots,
    std::optional<double> total_efficiency,
    double threshold)
{
    return enabled && !use_abyssal_hunter && room_slots > 0 && occupied_slots >= room_slots &&
           total_efficiency.has_value() && (*total_efficiency / room_slots) > threshold;
}
} // namespace asst::infrast
