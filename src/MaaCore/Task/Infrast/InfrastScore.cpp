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

#include "Config/Miscellaneous/BattleDataConfig.h"

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

// 跨设施组合
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

// 全局技能计算
bool has_skill(const SkillCounts& counts, std::string_view skill)
{
    return skill_count(counts, skill) > 0;
}

double floor_step(double value, double step)
{
    return std::floor(value / step) * step;
}

// 贸易站评分
CombinationScore score_trade(const std::vector<const ScoreOper*>& opers, const ScoreContext& context)
{
    const int max_storage = context.level == 1 ? 6 : context.level == 2 ? 8 : 10;
    double base = 0;
    int storage = 0;                     // 订单上限增减量
    int gold = context.gold_station_num; // 赤金生产线数，部分订单流技能会继续修正
    int alternate_orchid = 0;            // 焰狐龙梓兰的泡影国狩猎小队，暂时以焰狐龙梓兰标识
    int laterano = 0;                    // 同设施拉特兰干员数量
    int exusiai = 0;                     // 能天使相关计数
    const auto all = count_skills(opers);
    const bool is_money = context.product == "Money";
    CombinationScore result;

    // 输入组合，计算平均加成，与实际收益的差距：
    // 1. 只考虑 8 小时平均收益，非实际换班间隔；
    // 2. 心情阈值以下的干员不考虑，也忽略心情消耗；
    // 3. 无法稳定计算的概率效果忽略；
    // 4. 为了使某些组合优先，手动定义了部分技能的近似值，可能与实际略有差异。
    for (const auto* oper : opers) {
        for (const auto& icon : oper->skills) {
            if (icon == "bskill_tra_spd3") { // 物流专家 / 名流欢会：能天使、海蒂
                base += 0.35;
                if (is_operator(*oper, { "char_103_angel" })) {
                    ++laterano;
                    ++exusiai;
                }
            }
            else if (icon == "bskill_tra_spd&formula1") { // 精准排期：石英
                base += 0.34;                             // 近似两种产物
            }
            else if (icon == "bskill_tra_spd&meet1") {    // 新城贸易：伺夜
                base += 0.4;                              // 近似满级会客室
                // 八幡海铃在控制中枢时提高叙拉古干员的贸易站效率。
                if (is_selected(context, "char_4186_tmoris")) {
                    base += 0.05;
                }
            }
            else if (icon == "bskill_tra_ord_spd_ext2") { // 家族经营·α：贝洛内
                base += 0.25;
                if (is_selected(context, "char_4186_tmoris")) {
                    base += 0.05;
                }
            }
            else if (icon == "bskill_tra_ord_spd_ext3") { // 家族经营·β：贝洛内
                base += 0.3;
                if (is_selected(context, "char_4186_tmoris")) {
                    base += 0.05;
                }
            }
            else if (icon == "bskill_tra_share3") { // 勤俭经营·β：吉星
                base += 0.2 * (context.level - 1);
            }
            else if (icon == "bskill_tra_spd&cost") { // 交际：古米、空爆、月见夜
                base += 0.3;
            }
            else if (icon == "bskill_tra_spd&limit7") { // 使命必达 / 少当家等：可颂、拜松等
                base += 0.3;
                storage += 1;
            }
            else if (icon == "bskill_tra_spd&limit6") { // 供应管理：远山、玫兰莎、梓兰
                base += 0.25;
                storage += 1;
            }
            else if (icon == "bskill_ord_spd&tag1") {   // 精英小队：真言
                base += 0.25;                           // 精英干员设施数量暂不计算
            }
            else if (icon == "bskill_tra_spd&limit5") { // 喀兰之主：银灰
                base += 0.2;
                storage += 4;
                // 灵知将喀兰贸易技能统一转为容量收益。
                if (is_selected(context, "char_206_gnosis")) {
                    base -= 0.15;
                    storage += 6;
                }
            }
            else if (icon == "bskill_tra_share2") { // 勤俭经营·α：吉星
                base += 0.1 * (context.level - 1);
            }
            else if (icon == "bskill_tra_spd_bd2") { // 怅惘和声：黑键
                base += 0.005 * context.dormitory_capacity;
                // 迷迭香或絮雨已入驻时，补入对应的感知信息收益
                if (context.use_perception_information &&
                    (is_selected(context, "char_391_rosmon") || is_selected(context, "char_436_whispr"))) {
                    base += 0.44; // 手动定义，使之高于0.4
                }
            }
            else if (icon == "bskill_tra_spd&limit4") { // 喀兰贸易·β：崖心
                base += 0.15;
                storage += 4;
                if (is_selected(context, "char_206_gnosis")) {
                    base -= 0.15;
                    storage += 6;
                }
            }
            else if (icon == "bskill_tra_spd&limit3") { // 喀兰贸易·α：银灰、讯使、角峰
                base += 0.15;
                storage += 2;
                if (is_selected(context, "char_206_gnosis")) {
                    base -= 0.15;
                    storage += 6;
                }
            }
            else if (icon == "bskill_tra_spd&limit2") { // 订单管理·β：涤火杰西卡、四月
                base += 0.1;
                storage += 4;
            }
            else if (icon == "bskill_tra_spd&limit1") { // 订单管理·α：翎羽、四月、黑角
                base += 0.1;
                storage += 2;
            }
            else if (icon == "bskill_tra_closure") { // 特别订单：可露希尔
                base += 0.1;
            }
            else if (icon == "bskill_tra_spd2") { // 企鹅物流·β / 订单分发·β：空、芬等
                base += 0.3;
            }
            else if (icon == "bskill_tra_spd1") { // 订单分发·α / 企鹅物流·α等：多人共用
                base += 0.2;
            }
            else if (icon == "bskill_tra_par&per1") { // 白手起家·α：赫德雷
                base += 0.25;
                // 伊内丝在办公室时为赫德雷追加效率。
                if (is_selected(context, "char_4087_ines")) {
                    base += 0.05;
                }
            }
            else if (icon == "bskill_tra_par&per2") { // 白手起家·β：赫德雷
                base += 0.3;
                // 维什戴尔在控制中枢时增加订单上限，伊内丝在办公室时追加效率。
                if (is_selected(context, "char_1035_wisdel")) {
                    storage += 2;
                }
                if (is_selected(context, "char_4087_ines")) {
                    base += 0.05;
                }
            }
            else if (icon == "trade_ord_spd&par1" || icon == "trade_ord_spd&par2") { // 外贸决议：维娜·维多利亚
                base += 0.3;                                                         // 道格拉斯帮加成暂不计算
            }
            else if (icon == "bskill_tra_ord_spd_ext0") {                            // 对陆接洽代表·α：深巡
                base += 0.25;
                // 乌尔比安在制造站时追加效率。
                if (is_selected(context, "char_4145_ulpia")) {
                    base += 0.05;
                }
            }
            else if (icon == "bskill_tra_ord_spd_ext1") { // 对陆接洽代表·β：深巡
                base += 0.3;
                if (is_selected(context, "char_4145_ulpia")) {
                    base += 0.1;
                }
            }
            else if (icon == "bskill_tra_spd&meet") { // 天生的顾问：渡桥
                base += 0.15 + std::min(0.15, context.level * 0.05);
            }
            else if (icon == "bskill_tra_lemuen1") { // 相伴：蕾缪安
                base += 0.2;
                ++laterano;
            }
            else if (icon == "bskill_tra_spd3&catap2") { // 气氛组：雷狼龙S空爆
                base += 0.35;
                ++alternate_orchid;
            }
            else if (icon == "bskill_tra_spd&limit_down1") { // 威压：锏
                base += 0.2;
                storage -= 2;
                if (is_selected(context, "char_206_gnosis")) {
                    base -= 0.15;
                    storage += 6;
                }
            }
            else if (icon == "bskill_tra_spd&limit_down2") { // 不怒自威：锏
                base += 0.25;
                storage -= 6;
                if (is_selected(context, "char_206_gnosis")) {
                    base -= 0.15;
                    storage += 6;
                }
            }
            else if (icon == "bskill_tra_flow_gc2") { // 订单流可视化·β：绮良
                base += 0.05;
                gold += (gold / 2) * 2;
            }
            else if (icon == "bskill_tra_flow_gc1") { // 订单流可视化·α：绮良
                base += 0.05;
                gold += (gold / 4) * 2;
            }
            else if (icon == "bskill_tra_spd&limit_felyne") { // 可爱的艾露猫：泰拉大陆调查团
                base += 0.05;
                storage += 2;
                // 麒麟R夜刀与火龙S黑角同时在控制中枢时启用完整调查团加成。
                if (is_selected(context, "char_1029_yato2") && is_selected(context, "char_1030_noirc2")) {
                    base += 0.361; // 手动定义，使之shiao高于0.36
                }
            }
            else if (icon == "bskill_tra_spd&dorm2") { // 虔诚筹款·β：空弦
                base += 0.02 * context.dormitory_level_sum;
            }
            else if (icon == "bskill_tra_spd&dorm1") { // 虔诚筹款·α：空弦
                base += 0.01 * context.dormitory_level_sum;
            }
            else if (icon == "bskill_tra_bd_n2") { // “愿者上钩”：乌有
                base += 0.01 * context.dormitory_capacity;
                // 桑葚在办公室、重岳在控制中枢时分别补入人间烟火收益。
                if (context.use_worldly_plight && is_selected(context, "char_473_mberry")) {
                    base += 0.21;
                }
                if (context.use_worldly_plight && is_selected(context, "char_2024_chyue")) {
                    base += 0.15;
                }
            }
            else if (icon == "bskill_tra_limit&cost") { // 谈判：桃金娘、史都华德、暗索
                storage += 5;
            }
            else if (icon == "bskill_tra_limit&trade2") { // 钱不我待：瑰盐
                storage += context.level;
            }
            else if (icon == "bskill_tra_orchd2") { // 队长的自觉：焰狐龙梓兰
                storage += 3;
                ++alternate_orchid;
            }
            else if (icon == "bskill_tra_wt&cost2" && is_money) { // 裁缝·β / 手工艺品·β等：多人共用
                base += 0.02;                                     // 裁缝 B 单独使用效果很小，取近似值
            }
            else if (icon == "bskill_tra_wt&cost1" && is_money) { // 裁缝·α / 手工艺品·α等：多人共用
                base += 0.01;
            }
            else if (icon == "bskill_tra_long2" && is_money) { // 投资·β：龙舌兰
                base += 0.02;                                  // 投资 B 单独使用效果很小，取近似值
            }
            else if (icon == "bskill_tra_long1" && is_money) { // 投资·α：龙舌兰
                base += 0.01;
            }
        }
    }

    // 应用全局性技能效果

    if (has_skill(all, "bskill_tra_share1")) { // 代为说项：火哨
        base += (context.level - 1) * 0.15;
    }

    // 拉普兰德与德克萨斯
    const bool texas = has_skill(all, "bskill_tra_texas1") || has_skill(all, "bskill_tra_texas2"); // 德克萨斯
    if (has_skill(all, "bskill_tra_lappland1")) { // 醉翁之意·α：拉普兰德
        if (texas) {
            storage += 2;
            base += 0.65;
        }
        if (is_selected(context, "char_4186_tmoris")) {
            base += 0.05;
        }
    }
    else if (has_skill(all, "bskill_tra_lappland2")) { // 醉翁之意·β：拉普兰德
        if (texas) {
            storage += 4;
            base += 0.65;
        }
        if (is_selected(context, "char_4186_tmoris")) {
            base += 0.05;
        }
    }

    // 贝洛内与伺夜
    if (has_skill(all, "bskill_tra_spd&meet1") && has_skill(all, "bskill_tra_ord_spd_ext2")) {
        base += 0.05;
        storage += 2;
    }
    else if (has_skill(all, "bskill_tra_spd&meet1") && has_skill(all, "bskill_tra_ord_spd_ext3")) {
        base += 0.1;
        storage += 2;
    }

    // 焰狐龙梓兰相关技能
    if (const int count = skill_count(all, "bskill_tra_orchd2")) {
        base += alternate_orchid * 0.2 * count;
    }
    // 能天使与蕾缪安联动
    if (const int count = skill_count(all, "bskill_tra_lemuen1")) { // 相伴：蕾缪安
        base += exusiai * 0.25 * count;
    }

    if (const int count = skill_count(all, "bskill_tra_laterano1")) { // 同城加急单：新约能天使
        base += laterano * 0.15 * count;
    }
    if (const int count = skill_count(all, "bskill_tra_spd_variable22")) { // 天道酬勤·β：雪雉
        base += std::min(0.35, floor_step(base, 0.05)) * count;
    }
    if (const int count = skill_count(all, "bskill_tra_spd_variable21")) { // 天道酬勤·α：雪雉
        base += std::min(0.25, floor_step(base, 0.05)) * count;
    }
    if (const int count = skill_count(all, "bskill_tra_flow_gs2")) { // 物流规划·β：图耶
        base += 0.05 + (gold / 2) * 0.15 * count;
    }
    if (const int count = skill_count(all, "bskill_tra_flow_gs1")) { // 物流规划·α：图耶
        base += 0.05 + (gold / 4) * 0.15 * count;
    }
    if (const int count = skill_count(all, "bskill_tra_flow_gs")) { // 销路宣发：鸿雪
        base += gold * 0.05 * count;
    }
    if (has_skill(all, "bskill_trade_ord_spd_variable")) { // 招商引资：琳琅诗怀雅
        base += storage * 0.04;
    }
    if (has_skill(all, "bskill_tra_limit2spd")) { // 冠军风采：锏
        base += std::min(1.0, std::floor(storage / 5.0) * 0.25);
    }

    if (has_skill(all, "bskill_tra_limit_count")) { // 市井之道：孑
        base += std::max(1.0, max_storage + storage - std::floor(base / 0.1)) * 0.04;
    }
    else if (has_skill(all, "bskill_tra_limit_diff")) { // 摊贩经济：孑
        // 孑精零近似：一天三换，并且只在换班时收单。
        base += (max_storage + storage) * 0.04 / (context.level + 1.12) * 4.034;
    }

    // 巫恋组合会覆盖普通加成，并只保留组合需要的干员。
    if (has_skill(all, "bskill_tra_vodfox") && is_money) { // 低语：巫恋
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
        if (has_skill(all, "bskill_tra_against")) { // 违约索赔·α：但书
            base = (1 + base) * 1.276 - 1;
        }
        if (has_skill(all, "bskill_tra_against2")) { // 违约索赔·β：但书
            base = (1 + base) * 1.556 - 1;
        }
    }
    if (!has_skill(all, "bskill_tra_against") && !has_skill(all, "bskill_tra_against2") &&
        !has_skill(all, "bskill_tra_vodfox") && is_money && has_skill(all, "bskill_tra_closure")) {
        base = (1 + base) * 1.804 - 1;
    }

    if (has_skill(all, "bskill_tra_spd&wt1")) { // 天真的谈判者：U-Official
        base = -1;                              // 禁用尤里卡
    }
    result.value = base;
    return result;
}

// 制造站评分
CombinationScore score_mfg(const std::vector<const ScoreOper*>& opers, const ScoreContext& context)
{
    const int max_storage = context.level == 1 ? 24 : context.level == 2 ? 36 : 54;
    const bool records = context.product == "CombatRecord";     // 生产作战记录
    const bool gold_product = context.product == "PureGold";    // 生产赤金
    const bool origin_stone = context.product == "OriginStone"; // 生产源石碎片
    double base = 0;                                            // 通用基础加成
    double station = 0;                                         // 进设施加成
    int robot = 0;                                              // 工程机器人数量
    int standard = 0;                                           // 标准化技能数量
    int rhine = 0;                                              // 莱茵科技技能数量
    int metallics = 0;                                          // 金属工艺技能数量
    int abyssal_hunter = 0;                                     // 当前组合的深海猎人数
    bool station_only = false;                                  // 是否只使用设施数量加成
    std::vector<int> storage(opers.size(), 0);                  // 每名干员的仓储增减量
    const auto all = count_skills(opers);
    CombinationScore result;

    // 输入组合，原则上计算八小时平均加成，另有注明的近似除外：心情阈值以下的干员不参与枚举，
    // 不计算换班间隔和心情消耗，无法稳定量化的概率效果也不计入。
    // 一些组合效率相同时会调整让组合效率稍高一点，优先使用某些组合

    const bool abyssal_hunter_active = context.use_abyssal_hunter && is_selected(context, "char_474_glady");

    for (size_t index = 0; index < opers.size(); ++index) {
        const auto& oper = *opers[index];
        // 歌蕾蒂娅已在控制中枢时，斯卡蒂、幽灵鲨、乌尔比安、安哲拉组成制造站候选。
        if (abyssal_hunter_active &&
            is_operator(oper, { "char_263_skadi", "char_143_ghost", "char_4145_ulpia", "char_218_cuttle" })) {
            ++abyssal_hunter;
            if (abyssal_hunter < 3 && !is_selected(context, oper.operator_id)) {
                base += 0.401; // 手动调整，使之高于 0.4（槐琥、至简），能够优先上
            }
        }

        for (const auto& icon : oper.skills) {
            if (icon == "bskill_man_limit&cost5") { // 掘进工程：洋灰
                storage[index] += 10;
            }
            else if (icon == "bskill_man_exp3" && records) { // 拳术指导录像 / 逆境荣光：食铁兽、断罪者
                base += 0.35;
            }
            else if (icon == "bskill_man_exp2" && records) { // 作战指导录像 / 公证所教习·β：多人共用
                base += 0.3001;
            }
            else if ((icon == "bskill_formula_spd_headb1" || icon == "bskill_formula_spd_headb2") && records) {
                // 战阵领袖 / 情同手足：怒潮凛冬；乌萨斯学生团联动暂不计算。
                base += 0.3;
            }
            else if (icon == "bskill_man_exp1" && records) { // 胜利之计：帕拉斯
                base += 0.2501;
            }
            else if (icon == "bskill_man_exp0" && records) { // 公证所教习α：作战记录类配方生产力 +20%
                base += 0.2;
            }
            else if (icon == "bskill_man_exp&spd&limit&cost2" && records) { // 戏中人：酒神
                base += 0.35;
            }
            else if (icon == "bskill_man_exp&spd&limit&cost1" && records) { // 镜中影：酒神
                base += 0.2;
            }
            else if (icon == "bskill_man_exp&spd&cost1" && records) { // “连轴转”：裂响
                base += 0.34999;                                      // 手动调整
            }
            else if (icon == "bskill_man_gold2") {                    // 金属工艺·β：砾
                if (gold_product) {
                    base += 0.35;
                    ++metallics;
                }
                // 薇薇安娜在控制中枢时，为红松骑士团及砾追加制造效率。
                if (context.use_pinus_sylvestris && is_selected(context, "char_4098_vvana")) {
                    base += 0.07;
                }
            }
            else if (icon == "bskill_man_gold1" && gold_product) { // 金属工艺·α：夜烟、斑点等
                base += 0.3001;                                    // 手动调整增加0.001
                ++metallics;
            }
            else if (icon == "bskill_man_spd&dorm1" && gold_product) { // 齐心沙盗：娜仁图亚
                base += 0.01 * context.dormitory_level_sum;
            }
            else if (icon == "bskill_man_spd&trade" && gold_product) { // 再生能源：清流
                station += 0.2 * context.trading_station_num;
            }
            else if (icon == "bskill_man_spd&trade1" && gold_product) { // 原质塑金副产物：引星棘刺
                base += 0.03 * context.trading_station_num;
            }
            else if (icon == "bskill_man_gold3") { // 小奇思：杏仁
                if (gold_product) {
                    base += 0.25;
                }
                // 涤火杰西卡在控制中枢时追加黑钢体系收益。
                if (is_selected(context, "char_1034_jesca2")) {
                    base += 0.05;
                }
            }
            else if (icon == "bskill_man_gold&blacksteel" && gold_product) { // 挑大梁：杏仁
                base += 0.02;
                if (is_selected(context, "char_1034_jesca2")) {
                    base += 0.02;
                }
            }
            else if (icon == "bskill_man_spd_bd1") { // 念力：迷迭香
                if (context.use_perception_information && is_selected(context, "char_436_whispr")) {
                    base += 0.2;
                }
            }
            else if (icon == "bskill_man_spd_bd2") { // 意识实体：迷迭香
                // 絮雨在办公室时提供感知信息，迷迭香才获得完整制造效率。
                if (context.use_perception_information && is_selected(context, "char_436_whispr")) {
                    base += 0.40999; // 手动调整，使之高于0.4（槐琥）低于0.41（苍苔+引星棘刺）
                }
            }
            else if (icon == "bskill_man_spd_bd5") { // 逐水草：截云
                if (context.use_worldly_plight && is_selected(context, "char_473_mberry")) {
                    base += 0.4 / 5;
                }
                if (context.use_worldly_plight && is_selected(context, "char_2024_chyue")) {
                    base += 0.05 / 5;
                    if (is_selected(context, "char_2023_ling")) {
                        base += 0.05 / 5;
                    }
                    if (is_selected(context, "char_2015_dusk")) {
                        base += 0.05 / 5;
                    }
                }
                if (context.use_worldly_plight && is_selected(context, "char_2023_ling")) {
                    base += 0.15 / 5;
                }
            }
            else if (icon == "bskill_man_spd_bd6") { // 问枯荣：截云
                // 桑葚、重岳、令、夕分别提供人间烟火；技能按 2.5 点信息折算效率。
                if (context.use_worldly_plight && is_selected(context, "char_473_mberry")) {
                    base += 0.4 / 2.5;
                }
                if (context.use_worldly_plight && is_selected(context, "char_2024_chyue")) {
                    base += 0.05 / 2.5;
                    if (is_selected(context, "char_2023_ling")) {
                        base += 0.05 / 2.5;
                    }
                    if (is_selected(context, "char_2015_dusk")) {
                        base += 0.05 / 2.5;
                    }
                }
                if (context.use_worldly_plight && is_selected(context, "char_2023_ling")) {
                    base += 0.15 / 2.5;
                }
            }
            else if (icon == "bskill_man_spd_bd7") { // 稻禾厚，顺秋收：黍
                // 黍自身已占 1 点人间烟火，其他成员贡献按 3 点信息折算效率。
                if (context.use_worldly_plight && is_selected(context, "char_473_mberry")) {
                    base += 0.4 / 3;
                }
                if (context.use_worldly_plight && is_selected(context, "char_2024_chyue")) {
                    base += 0.1 / 3;
                    if (is_selected(context, "char_2023_ling")) {
                        base += 0.05 / 3;
                    }
                    if (is_selected(context, "char_2015_dusk")) {
                        base += 0.05 / 3;
                    }
                }
                if (context.use_worldly_plight && is_selected(context, "char_2023_ling")) {
                    base += 0.15 / 3;
                }
            }
            else if (icon == "bskill_man_spd3") { // 咪波·制造型 / 莱茵科技·γ：梅尔、淬羽赫默
                base += 0.3;
                if (is_operator(oper, { "char_1031_slent2" })) {
                    ++rhine;
                }
            }
            else if (icon == "bskill_man_spd2") { // 标准化·β / 莱茵科技·β / 红松骑士团·β
                base += 0.25;
                // 标准化：史都华德、香草、调香师、杰西卡、水月、罗比菈塔、海沫。
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
                // 莱茵科技：白面鸮、赫默、多萝西、星源。
                else if (is_operator(
                             oper,
                             { "char_128_plosis", "char_108_silent", "char_4048_doroth", "char_135_halo" })) {
                    ++rhine;
                }
                // 红松骑士团：远牙、灰毫、野鬃；受薇薇安娜、焰尾和正义骑士号联动影响。
                else if (
                    context.use_pinus_sylvestris &&
                    is_operator(oper, { "char_430_fartth", "char_431_ashlok", "char_496_wildmn" })) {
                    if (is_selected(context, "char_4098_vvana")) {
                        base += 0.07;
                    }
                    if (is_selected(context, "char_420_flamtl")) {         // 焰尾在控制中枢时为红松骑士团干员追加效率
                        base += records ? 0.101 : gold_product ? -0.1 : 0; // 手动调整增加0.001
                    }
                    if (is_selected(context, "char_4000_jnight") && is_operator(oper, { "char_496_wildmn" })) {
                        base += 0.052;
                    }
                }
                // 涤火杰西卡在控制中枢时，为香草、杰西卡追加黑钢体系收益。
                if (is_selected(context, "char_1034_jesca2") &&
                    is_operator(oper, { "char_240_wyvern", "char_235_jesica" })) {
                    base += 0.05;
                }
            }
            else if (icon == "bskill_man_limit&cost3") { // 探险者 / 收纳达人：石棉
                storage[index] += 16;
            }
            else if (icon == "bskill_man_spd&limit&cost3") { // 特立独行 / 麻烦制造者：泡普卡、石棉
                base += 0.25;
                storage[index] -= 12;
            }
            else if (icon == "bskill_man_spd&limit6") { // 行动派
                base += 0.2;
                storage[index] -= 8;
            }
            else if (icon == "bskill_man_spd_add1") { // 急性子 / “等不及”：芬、刻俄柏
                base += 0.23125;
            }
            else if (icon == "bskill_man_spd_veen") {   // 手艺人：维伊
                base += 0.299;                          // 认为训练室三级
            }
            else if (icon == "bskill_man_spd_reduce") { // 模糊视线：铅踝
                base += 0.22;
            }
            else if (icon == "bskill_man_spd_add2") { // 慢性子 / 延时摄影：克洛丝、稀音
                base += 0.2125;
            }
            else if (icon == "bskill_man_marcille1") { // 差遣使魔·α：玛露西尔
                base += 0.2;
            }
            else if (icon == "bskill_man_marcille2") { // 差遣使魔·β：玛露西尔
                base += 0.3;
            }
            else if (icon == "bskill_man_spd&cost1" || icon == "bskill_man_spd&cost2") {
                // 量体裁衣：裁度；虔信：雪猎。
                base += 0.2;
            }
            else if (icon == "bskill_man_spd&cost3") { // 独当一面：雪猎
                base += 0.3;
            }
            else if (icon == "bskill_man_spd&limit5") { // 得心应手：裁度
                base += 0.25;
                storage[index] += 6;
            }
            else if (icon == "bskill_man_spd_add3") { // 例行清扫：阿罗玛
                base += 0.1;                          // 十小时平均收益，八小时收益约 0.073
            }
            else if (icon == "bskill_man_spd1") {     // 标准化·α / 莱茵科技·α / 红松骑士团·α
                base += 0.15;
                // 标准化：夜刀、流星、豆苗、褐果、石英、洋灰、历阵锐枪芬。
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
                // 莱茵科技：白面鸮、赫默、星源。
                else if (is_operator(oper, { "char_128_plosis", "char_108_silent", "char_135_halo" })) {
                    ++rhine;
                }
                // 红松骑士团：远牙、灰毫、野鬃；联动规则与 β 技能相同。
                else if (
                    context.use_pinus_sylvestris &&
                    is_operator(oper, { "char_430_fartth", "char_431_ashlok", "char_496_wildmn" })) {
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
            else if (icon == "bskill_man_spd&limit3") { // 仓库整备·β：蛇屠箱、黑角
                base += 0.1;
                storage[index] += 10;
            }
            else if (icon == "bskill_man_spd&limit1") { // 仓库整备·α：米格鲁、卡缇
                base += 0.1;
                storage[index] += 6;
            }
            else if (icon == "bskill_man_spd&limit&cost2") { // 工匠精神·β：火神
                base -= 0.05;
                storage[index] += 19;
            }
            else if (icon == "bskill_man_spd&limit&cost1") { // 工匠精神·α：火神
                base -= 0.05;
                storage[index] += 16;
            }
            else if (icon == "bskill_man_spd&limit&cost4") { // “可靠”助手：贝娜
                base -= 0.2;
                storage[index] += 17;
            }
            else if (icon == "bskill_man_fuze") { // 实干的寡言者：导火索
                base += 0.2;
            }
            else if (icon == "bskill_man_gold4" && gold_product) { // 净味香氛：阿罗玛
                base += 0.25;
            }
            else if (icon == "bskill_man_exp&limit2" && records) { // 剪辑·β：卡达
                storage[index] += 15;
            }
            else if (icon == "bskill_man_exp&limit1" && records) { // 剪辑·α：稀音
                storage[index] += 12;
            }
            else if (icon == "bskill_man_exp&limit3" && records) { // 合理利用：圣约送葬人
                base -= 0.0001;
                storage[index] += 4;
            }
            else if (icon == "bskill_man_limit&cost2") { // 囤积者 / 无畏豪情等：泡泡、娜仁图亚
                storage[index] += 10;
            }
            else if (icon == "bskill_man_limit&cost1") { // 拾荒者 / 磐蟹·阿盘等：多人共用
                storage[index] += 8;
            }
            else if (icon == "bskill_man_originium2" && origin_stone) { // 源石工艺·β / 地质学·β等：多人共用
                base += 0.35;
            }
            else if (icon == "bskill_man_originium1" && origin_stone) { // 源石工艺·α / 地质学·α：多人共用
                base += 0.3001;
            }
            else if (icon == "bskill_man_token_spd1" && gold_product) { // 机械精通·α：阿兰娜
                base += 0.05 * context.workbench_num;
            }
            else if (icon == "bskill_man_token_spd2" && gold_product) { // 机械精通·β：阿兰娜
                base += 0.1 * context.workbench_num;
            }
            else if (icon == "bskill_man_constrlv") { // 绘图设计：至简
                robot = std::min(64, robot + context.total_station_level);
            }
        }
    }

    if (has_skill(all, "bskill_man_spd_bd3")) { // 机械辅助·α：至简
        base += (robot / 16) * 0.05;
    }
    if (has_skill(all, "bskill_man_spd_bd4")) { // 机械辅助·β：至简
        base += (robot / 8) * 0.05;
    }

    const bool tragodia =
        has_skill(all, "bskill_man_exp&spd&limit&cost2") || has_skill(all, "bskill_man_exp&spd&limit&cost1");
    if (has_skill(all, "bskill_man_spd_double2") && tragodia && records) { // 盛餐的回报：Miss.Christine
        base += 0.3;
    }

    if (const int bubble_count = skill_count(all, "bskill_man_spd_variable31")) { // 大就是好！：泡泡
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
    else if (const int vermeil_count = skill_count(all, "bskill_man_spd_variable11")) { // 回收利用：红云
        const int sum = std::max(0, std::accumulate(storage.begin(), storage.end(), 0));
        base += sum * 0.02 * vermeil_count;
    }
    if (const int count = skill_count(all, "bskill_man_spd_variable21")) {            // 配合意识：槐琥
        base += std::min(0.4, floor_step(base, 0.05) - abyssal_hunter * 0.4) * count; // 槐琥和深海猎人共存时不起作用
    }

    // 发电站数量类技能只与同类设施加成组合，白板位置不强制占用。
    if (const int count = skill_count(all, "bskill_man_spd&power3")) { // 仿生海龙：温蒂
        station_only = true;
        station += 0.16 * context.virtual_power_station_num * count;
        result.only_need.emplace("bskill_man_spd&power3");
    }
    if (const int count = skill_count(all, "bskill_man_spd&power2")) { // 自动化·β：温蒂、森蚺
        station_only = true;
        station += 0.11 * context.virtual_power_station_num * count;
        result.only_need.emplace("bskill_man_spd&power2");
    }
    if (const int count = skill_count(all, "bskill_man_spd&power1")) { // 自动化·α：森蚺、掠风、异客
        station_only = true;
        station += 0.06 * context.virtual_power_station_num * count;
        result.only_need.emplace("bskill_man_spd&power1");
    }
    if (has_skill(all, "bskill_man_spd_manu2")) { // 流程优化：冬时；仓储加成不计算
        station_only = true;
        station += 0.1 * context.level;
        result.only_need.emplace("bskill_man_spd_manu2");
    }
    if (const int count = skill_count(all, "bskill_man_skill_spd")) { // 意识协议：水月，按标准化技能数加成
        base += standard * 0.0501 * count - 0.0002;
    }
    if (const int count = skill_count(all, "bskill_man_skill_spd2")) { // 源石技艺理论应用：多萝西，按莱茵科技技能数加成
        base += rhine * 0.0501 * count - 0.0002;
    }
    if (const int count = skill_count(all, "bskill_man_gold&rhine"); count && gold_product) {
        // 造价高昂：娜斯提；全基建莱茵干员数暂按当前组合的莱茵技能数近似。
        base += rhine * 0.03 * count;
    }
    if (const int count = skill_count(all, "bskill_man_skill_spd3"); count && gold_product) {
        // 打工心得：苍苔，按金属工艺技能数加成。
        base += metallics * 0.0501 * count - 0.0002;
    }

    const int storage_sum = std::accumulate(storage.begin(), storage.end(), 0);
    // 容量低于 20 或累计减容量超过 15 的组合禁用，防止过小容量和多次减容量。
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

// 办公室评分
double office_score(const ScoreOper& oper)
{
    double score = 0;
    for (const auto& icon : oper.skills) {
        if (icon == "bskill_hire_skgoat2") { // 法为正典：斥罪
            score += 0.5;
        }
        else if (icon == "bskill_hire_skgoat3") { // 永不停歇·α：水灯心
            score += 0.3;
        }
        else if (icon == "bskill_hire_skgoat4" || icon == "bskill_hire_skgoat") {
            // 永不停歇·β：水灯心；准时下班：地灵。
            score += 0.45;
        }
        else if (icon == "bskill_hire_spd5") { // 天灾信使·β / 节目邀约等：普罗旺斯、艾雅法拉
            score += 0.45;
        }
        else if (icon == "bskill_hire_spd4") { // WRITER / B-girl / 心理学等：多人共用
            score += 0.4;
        }
        else if (icon == "bskill_hire_spd3") { // 人事管理·β：伊内丝
            score += 0.35;
        }
        else if (icon == "bskill_hire_spd&clue") { // 洞悉人心 / 好事之徒：月禾、乌有
            score += 0.36;
        }
        else if (icon == "bskill_hire_spd2" || icon == "bskill_ws_p_kalts2") {
            // 天灾信使·α等：多人共用；后者为凯尔希·思衡托的专属技能。
            score += 0.3;
        }
        else if (icon == "bskill_hire_spd&ursus2") { // 人望：早露
            score += 0.2;
        }
        else if (icon == "bskill_hire_spd_memento") { // 追忆：絮雨
            score += 0.31;
        }
        else if (icon == "bskill_hire_spd_bd_n2") { // 救援队·灾后普查：桑葚
            score += 0.301;
        }
        // 内幕：山；巡游：絮雨；救援队·资源清点：桑葚；语言学：闪击；人事管理·α：巡林者。
        else if (
            icon == "bskill_hire_spd&blacksteel2" || icon == "bskill_hire_spd_bd_n1_n1" ||
            icon == "bskill_hire_spd&cost2" || icon == "bskill_hire_blitz" || icon == "bskill_hire_spd1") {
            score += 0.2;
        }
        else if (icon == "bskill_hire_spd&cost1" || icon == "bskill_hire_spd") {
            // 救援队·珠算：桑葚；普通 10% 人脉资源联络速度技能。
            score += 0.1;
        }
        else if (icon == "bskill_hire_spd&char1") { // 雪境归心：圣聆初雪
            score += 0.35;                          // 控制中枢联动暂不计算
        }
        else if (icon == "bskill_hire_spd&clue2") { // 交游广阔：隐现
            score += 0.395;
        }
        else if (icon == "bskill_hire_spd&clue3") { // “号外！”：乌啾
            score += 0.395;
        }
        else if (icon == "bskill_hire_spd&clue4") { // 街头法则：乌啾
            score += 0.445;
        }
        else if (icon == "bskill_hire_spd&cost3") { // 救援队·保证体力：雪绒
            score += 0.35;
        }
        else if (icon == "bskill_hire_spd&cost4") { // 特殊渠道 / 踏坊寻味·α：林、行箸
            score += 0.2;
        }
        else if (icon == "bskill_hire_spd&cost5") { // 宫廷礼仪：深律
            score += 0.1;
        }
        else if (icon == "bskill_hire_spd&cost6") { // 威权谕使 / 踏坊寻味·β：深律、行箸
            score += 0.3;
        }
        else if (icon == "bskill_hire_spd&cost7") { // 圣女声望：圣聆初雪
            score += 0.2;
        }
        else if (icon == "bskill_hire_spd&dorm1") { // 梅兰德侦探·α：锡人
            score += 0.249;
        }
        else if (icon == "bskill_hire_spd&dorm2") { // 梅兰德侦探·β：锡人
            score += 0.449;
        }
        else if (icon == "bskill_hire_spd&extra1") { // 用人唯才：林
            score += 0.2;
        }
        else if (icon == "bskill_hire_spd&glasgow") { // 旧识新交：戴菲恩
            score += 0.2;
        }
    }
    return score;
}

// 发电站评分
double power_score(const ScoreOper& oper, const ScoreContext& context)
{
    double score = 0;
    // 作业平台：Lancet-2、Castle-3、THRM-EX、正义骑士号、Friston-3、PhonoR-0。
    const bool robot = is_operator(
        oper,
        { "char_285_medic2",
          "char_286_cast3",
          "char_376_therex",
          "char_4000_jnight",
          "char_4093_frston",
          "char_4136_phonor" });
    for (const auto& icon : oper.skills) {
        if (icon == "bskill_pow_count") { // 晨曦：承曦格雷伊，将发电站数量视为额外增加一座
            score += (is_selected(context, "char_4000_jnight") || context.workbench_num > 0) ? -1 : 0.1;
        }
        else if (icon == "bskill_pow_spd1") { // 各类 10% 充能技能：多人及作业平台共用
            score += 0.1;
            // 承曦格雷伊在场时，作业平台应留给其发电站计数联动，不按普通充能技能选择。
            if (robot && is_selected(context, "char_1027_greyy2")) {
                score -= 1;
            }
            // 没有承曦格雷伊时，正义骑士号可与 Lancet-2 组成作业平台联动。
            if (is_operator(oper, { "char_285_medic2" }) && is_selected(context, "char_4000_jnight") &&
                !is_selected(context, "char_1027_greyy2")) {
                score += 0.101;
            }
        }
        else if (icon == "bskill_pow_spd_p1") { // 咒文共鸣：PhonoR-0
            if (is_selected(context, "char_1027_greyy2")) {
                score -= 1;
            }
        }
        else if (icon == "bskill_power_rec_spd&addition2") { // 技术交流·β：空构
            score += 0.15;
        }
        else if (icon == "bskill_power_rec_spd&addition1") { // 技术交流·α：空构
            score += 0.13;
        }
        else if (icon == "bskill_power_rec_rhine") { // 生态科主任：缪尔赛思
            score += 0.11;
        }
        else if (icon == "bskill_pow_drone") { // 巡线框架：承曦格雷伊，按无人机上限折算
            score += 0.22;
        }
        else if (icon == "bskill_pow_spd3") { // 各类 20% 充能技能：多人共用
            score += 0.2;
            if (robot) {
                score -= 0.2; // 防止作业平台的相似图标误判
            }
        }
        else if (icon == "bskill_pow_spd2") { // 各类 15% 充能技能：多人共用
            score += 0.15;
            // 异客和清流优先留给制造站设施联动，不在发电站消耗。
            if (is_operator(oper, { "char_472_pasngr", "char_385_finlpp" })) {
                score -= 1;
            }
        }
        else if (icon == "bskill_pow_spd&cost") { // 热情澎湃 / 外卖水果挞：THRM-EX、空构
            // 凯尔希在控制中枢时补入作业平台效率。
            if (is_selected(context, "char_003_kalts")) {
                score += 0.05;
            }
            if (is_selected(context, "char_1027_greyy2")) {
                score -= 1;
            }
        }
        else if (icon == "bskill_pow_jnight") { // “滴滴，启动！”：正义骑士号
            if (is_selected(context, "char_1027_greyy2")) {
                score -= 1;
            }
            // 焰尾或薇薇安娜在控制中枢时，启用红松骑士团联动。
            else if (
                context.use_pinus_sylvestris &&
                (is_selected(context, "char_420_flamtl") || is_selected(context, "char_4098_vvana"))) {
                score += 0.101;
            }
        }
    }
    return score;
}

// 加工站材料合成评分：product 为材料 ID，level 为材料品质等级。
double processing_score(const ScoreOper& oper, const ScoreContext& context)
{
    const std::string_view material_id = context.product;
    double score = 0;
    for (const auto& icon : oper.skills) {
        if (icon == "bskill_ws_asc1" && material_id.size() == 4 && material_id.starts_with("32")) {
            score += 0.7;
        }
        else if (icon == "bskill_ws_asc2" && material_id.size() == 4 && material_id.starts_with("32")) {
            score += 0.8;
        }
        else if (icon == "bskill_hire_kalts2" || icon == "bskill_ws_p_kalts2") {
            score += 0.8;
        }
        else if (icon == "bskill_ws_p5") {
            continue;
        }
        else if (icon == "bskill_ws_p4") {
            score += 0.65;
        }
        else if (icon == "bskill_ws_p3") {
            score += 0.6;
        }
        else if (icon == "bskill_ws_evolve4") {
            score += 1.0;
        }
        else if (icon == "bskill_ws_evolve3") {
            score += 0.8;
        }
        else if (icon == "bskill_ws_evolve2") {
            score += 0.75;
        }
        else if (icon == "bskill_ws_evolve1") {
            score += 0.7;
        }
        else if (icon == "bskill_ws_free") {
            score += 0.8 - context.level * 0.1;
        }
        else if (icon == "bskill_ws_cost_blemishine") {
            score += 0.4;
        }
        else if (icon == "bskill_ws_bonus1" && context.level < 4) {
            score += 0.9;
        }
        else if (icon == "bskill_ws_bonus2" && context.level == 4) {
            score += 0.9;
        }
        else if (icon == "bskill_ws_alloyblock" && material_id == "31024") {
            score += 1.0;
        }
        else if (icon == "bskill_ws_orirock" && (material_id == "30014" || material_id == "30013")) {
            score += 0.9;
        }
        else if (icon == "bskill_ws_device" && (material_id == "30064" || material_id == "30063")) {
            score += 0.9;
        }
        else if (icon == "bskill_ws_crystalline" && (material_id == "31034" || material_id == "30145")) {
            score += 0.8;
        }
        else if (icon == "bskill_ws_skill3" && (material_id == "3302" || material_id == "3303")) {
            score += 1.8;
        }
        else if (icon == "bskill_ws_skill2" && (material_id == "3302" || material_id == "3303")) {
            score += 1.75;
        }
        else if (icon == "bskill_ws_skill1" && (material_id == "3302" || material_id == "3303")) {
            score += 1.7;
        }
    }
    return score;
}

ScoreResult select_single(const std::vector<ScoreOper>& opers, const ScoreContext& context)
{
    ScoreResult result;
    for (const size_t index : eligible_indices(opers, context)) {
        if (context.facility == "Office" && !context.use_perception_information &&
            (has_skill(opers[index], "bskill_hire_spd_memento") || is_operator(opers[index], { "char_436_whispr" }))) {
            continue;
        }
        if (context.facility == "Office" && !context.use_worldly_plight &&
            (has_skill(opers[index], "bskill_hire_spd_bd_n2") || is_operator(opers[index], { "char_473_mberry" }))) {
            continue;
        }
        double score = 0;
        if (context.facility == "Office") {
            score = office_score(opers[index]);
        }
        else if (context.facility == "Power") {
            score = power_score(opers[index], context);
        }
        else if (context.facility == "Processing") {
            score = processing_score(opers[index], context);
        }
        if (result.indices.empty() || score > result.score) {
            result.indices = { index };
            result.score = score;
        }
    }
    return result;
}

// 会客室评分
ScoreResult select_reception(const std::vector<ScoreOper>& opers, const ScoreContext& context)
{
    // 会客室先选专属高收益技能，其余候选保持原识别顺序。
    std::vector<size_t> priority;
    std::vector<size_t> preferred;
    std::vector<size_t> remain;
    for (const size_t index : eligible_indices(opers, context)) {
        const auto& oper = opers[index];
        if (has_skill(oper, "bskill_meet_spdowned1")) { // 显眼的调查者：U-Official
            continue;                                   // 禁用尤里卡
        }
        // 见行者、跃跃固定优先；菲亚梅塔已入驻时，信仰搅拌机也进入最高优先级。
        if (has_any_skill(oper, { "bskill_meet_spd&cost", "bskill_meet_exchange" }) ||
            (has_skill(oper, "bskill_meet_spd_confes1") && is_selected(context, "char_300_phenxi"))) {
            priority.emplace_back(index);
        }
        // 晓歌、伊内丝优先于普通干员；通用 25% 技能排除伺夜，避免占用其贸易站联动。
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

// 控制中枢选择干员
ScoreResult select_control(const std::vector<ScoreOper>& opers, const ScoreContext& context)
{
    // 控制中枢按固定优先级选择；同类制造加速、贸易加速、其他设施心情减免、办公室加速不重复占位。
    auto eligible = eligible_indices(opers, context);
    if (!context.use_pinus_sylvestris) {
        std::erase_if(eligible, [&](size_t index) {
            return has_any_skill(opers[index], { "bskill_ctrl_psk", "bskill_ctrl_fraction_knight" }) ||
                   is_operator(opers[index], { "char_420_flamtl", "char_4098_vvana" });
        });
    }
    if (!context.use_abyssal_hunter) {
        std::erase_if(eligible, [&](size_t index) {
            return has_skill(opers[index], "bskill_ctrl_aegir2") || is_operator(opers[index], { "char_474_glady" });
        });
    }
    std::vector<size_t> best;
    bool manu_acc = false;    // 制造加速
    bool trading_acc = false; // 贸易加速
    bool mood_reduce = false; // 其他设施心情减免
    bool office_acc = false;  // 办公室加速
    const bool perception_information = context.use_perception_information && is_selected(context, "char_436_whispr");
    const bool worldly_plight = context.use_worldly_plight && is_selected(context, "char_473_mberry");
    constexpr size_t ControlSlotCount = 5;
    // context.slots 在缺员复查时是本轮仍需补入的人数；控制中枢的实际槽位始终是 5。
    const size_t selection_limit = std::min(ControlSlotCount, static_cast<size_t>(std::max(0, context.slots)));

    auto contains = [&](size_t index) {
        return std::ranges::find(best, index) != best.end();
    };
    auto has_room_for = [&](size_t count) {
        return best.size() <= selection_limit && count <= selection_limit - best.size();
    };
    auto add_first = [&](const auto& predicate) {
        if (!has_room_for(1)) {
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

    // 诗怀雅与斩业星熊的龙门近卫局制造加速必须同时存在，否则整组放弃。
    // 诗怀雅的 bskill_ctrl_t_spd 与多名干员共用，必须依赖姓名 OCR 得到稳定角色 ID。
    if (best.size() < 2 && has_room_for(2)) {
        const auto swire = std::ranges::find_if(eligible, [&](size_t index) {
            return is_operator(opers[index], { "char_308_swire" });
        });
        const auto guard = std::ranges::find_if(eligible, [&](size_t index) {
            return has_skill(opers[index], "bskill_token_prod_spd3_lungmenguard"); // 共事情谊：斩业星熊
        });
        if (swire != eligible.end() && guard != eligible.end() && *swire != *guard) {
            best.emplace_back(*swire);
            best.emplace_back(*guard);
            trading_acc = true;
            manu_acc = true;
        }
    }

    // 麒麟R夜刀与火龙S黑角组合：前者固定制造加速，后者按技能阶段提供贸易加速。
    if (best.size() <= 2 && !manu_acc && !trading_acc && has_room_for(2)) {
        const auto yato = std::ranges::find_if(eligible, [&](size_t index) {
            return has_skill(opers[index], "bskill_ctrl_token_p_spd2") && // 以身作则
                   has_skill(opers[index], "bskill_ctrl_cost_felyne");    // 耐力回复
        });
        const auto noir = std::ranges::find_if(eligible, [&](size_t index) {
            // 秘传交涉术 / 团队合作。
            return has_any_skill(opers[index], { "bskill_ctrl_token_t_spd", "bskill_ctrl_felyne" });
        });
        if (yato != eligible.end() && noir != eligible.end() && *yato != *noir) {
            best.emplace_back(*yato);
            best.emplace_back(*noir);
            manu_acc = true;
            trading_acc = has_skill(opers[*noir], "bskill_ctrl_token_t_spd");
        }
    }

    // 合作协议 / 大小姐 / 朝气蓬勃 / 情报主脑：阿米娅、诗怀雅、明椒、阿斯卡纶；
    // 权变：望。两类技能均只占用一次贸易加速名额。
    if (best.size() <= 3 && !trading_acc && add_first([](const ScoreOper& oper) {
            return has_any_skill(oper, { "bskill_ctrl_t_spd", "bskill_ctrl_tra&prod" });
        })) {
        trading_acc = true;
    }

    if (best.size() <= 4) {
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_psk"); }); // 红松的骑士：焰尾
        add_first([](const ScoreOper& oper) {
            return has_skill(oper, "bskill_ctrl_fraction_knight");                           // 烛骑士微光：薇薇安娜
        });
    }

    // 絮雨在办公室时，选择高心情的夕提供感知信息。
    if (best.size() < ControlSlotCount && perception_information) {
        add_first([](const ScoreOper& oper) {
            // “不以物喜” + “不以己悲”：夕。
            return has_skill(oper, "bskill_ctrl_cost_bd1") && has_skill(oper, "bskill_ctrl_cost_bd2") &&
                   oper.mood_ratio > 22.0 / 24.0;
        });
    }
    // 桑葚在办公室时，选择高心情的令提供人间烟火。
    if (best.size() < ControlSlotCount && worldly_plight) {
        add_first([](const ScoreOper& oper) {
            return has_skill(oper, "bskill_ctrl_cost_bd1&bd2") && // “山河远阔”：令
                   oper.mood_ratio > 22.0 / 24.0;
        });
    }

    // 深海队只有在选项开启且不会挤掉完整发电/骑士联动时使用，高心情是必要条件。
    if (best.size() < ControlSlotCount && context.use_abyssal_hunter &&
        !(is_selected(context, "char_1027_greyy2") && is_selected(context, "char_420_flamtl") &&
          is_selected(context, "char_4098_vvana"))) {
        add_first([](const ScoreOper& oper) {
            return has_skill(oper, "bskill_ctrl_aegir2") && // 集群狩猎·β：歌蕾蒂娅
                   oper.mood_ratio > 22.0 / 24.0;
        });
    }

    // 感知信息或人间烟火组合需要琴柳补办公室加速。
    if (best.size() < ControlSlotCount && (perception_information || worldly_plight) &&
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_h_spd"); })) { // 感染力：琴柳
        office_acc = true;
    }
    if (best.size() < ControlSlotCount && !office_acc &&
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_hire_tmoris"); })) {
        // 可靠伙伴：八幡海铃；同时影响后续叙拉古干员的效率计算。
        office_acc = true;
    }
    if (best.size() < ControlSlotCount && !manu_acc &&
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_p_spd"); })) {
        // 最高权限：凯尔希；同类制造加速只选择一次。
        manu_acc = true;
    }

    // 桑葚在办公室时，重岳提供人间烟火并承担全局心情减免。
    if (best.size() < ControlSlotCount && worldly_plight &&
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_cost_bd3"); })) { // 知我为我：重岳
        mood_reduce = true;
    }
    // 人间烟火组合已有重岳后，才继续补夕。
    if (best.size() < ControlSlotCount &&
        std::ranges::any_of(best, [&](size_t index) { return is_operator(opers[index], { "char_2024_chyue" }); })) {
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_cost_bd1"); }); // “不以物喜”：夕
    }

    // 丰川祥子技能可与制造加速叠加，后续同团成员按固定顺序补入。
    const bool oblivionis = best.size() < ControlSlotCount && add_first([](const ScoreOper& oper) {
                                return has_skill(oper, "bskill_ctrl_p_oblvns"); // 丰富工作经验：丰川祥子
                            });
    if (oblivionis || is_selected(context, "char_4182_oblvns")) {
        if (best.size() < ControlSlotCount) {
            add_first([](const ScoreOper& oper) {
                return has_skill(oper, "bskill_ctrl_trade_mortis"); // 演技的怪物：若叶睦
            });
        }
        if (best.size() < ControlSlotCount) {
            add_first([](const ScoreOper& oper) {
                return has_skill(oper, "bskill_ctrl_dorm_uika1"); // 偶像光环：三角初华
            });
        }
        if (best.size() < ControlSlotCount) {
            add_first([](const ScoreOper& oper) {
                return has_skill(oper, "bskill_ctrl_meet_amoris1"); // 勤学苦练：祐天寺若麦
            });
        }
    }

    if (best.size() < ControlSlotCount && !manu_acc && context.workbench_num > 1 &&
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_token_p_spd"); })) { // 超频：布丁
        manu_acc = true;
    }
    if (best.size() < ControlSlotCount && !perception_information && !worldly_plight) {
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_t_limit&spd"); }); // 精密计算：灵知
    }

    if (best.size() < ControlSlotCount && !mood_reduce) {
        // 孤光共照：重岳；公事公办：玛恩纳；巴别塔之帜：维什戴尔。
        // 三者都承担全局心情减免，因此只选第一个可用者。
        if (add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_cost_bd4"); }) ||
            add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_lonely"); }) ||
            add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_cost_expand"); })) {
            mood_reduce = true;
        }
    }

    if (best.size() < ControlSlotCount && !perception_information && !worldly_plight &&
        is_selected(context, "char_285_medic2")) {
        // 森蚺只在 Lancet-2 已入驻发电站时加入候选，启用作业平台联动。
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_p_bot"); }); // 我寻思能行：森蚺
    }
    if (best.size() < ControlSlotCount) {
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_bd_spd"); }); // 老友相聚：涤火杰西卡
    }
    if (best.size() < ControlSlotCount && is_selected(context, "char_1035_wisdel")) {
        // 维什戴尔已在贸易站联动链中时，魔王的期冀之汇才加入候选。
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_c_spd1"); }); // 期冀之汇：魔王
    }
    if (best.size() < ControlSlotCount &&
        !std::ranges::any_of(best, [&](size_t index) { return is_operator(opers[index], { "char_474_glady" }); })) {
        // 艾拉与歌蕾蒂娅不同时占用控制中枢位置，避免挤掉完整的深海组合。
        add_first([](const ScoreOper& oper) { return has_skill(oper, "bskill_ctrl_ela"); }); // 反抗者：艾拉
    }

    // 玛恩纳在场时尽量补满“笑脸”类技能，但排除凯尔希，避免重复制造加速。
    const bool mlynar =
        std::ranges::any_of(best, [&](size_t index) { return is_operator(opers[index], { "char_4064_mlynar" }); });
    if (best.size() < ControlSlotCount && mlynar) {
        while (add_first([](const ScoreOper& oper) {
            return has_skill(oper, "bskill_ctrl_cost") && !has_skill(oper, "bskill_ctrl_p_spd");
        })) {
        }
    }
    // 剩余位置按异格者与普通“笑脸”技能补齐。
    if (best.size() < ControlSlotCount) {
        while (add_first(
            [](const ScoreOper& oper) { return has_any_skill(oper, { "bskill_ctrl_sp", "bskill_ctrl_cost" }); })) {
        }
    }
    // 仍未填满时，用其余可用且未选择的干员补齐控制中枢。
    if (best.size() < ControlSlotCount) {
        while (add_first([](const ScoreOper&) { return true; })) {
        }
    }

    const double score = static_cast<double>(best.size());
    return { std::move(best), score };
}

ScoreResult select_dorm(const std::vector<ScoreOper>& opers, const ScoreContext& context)
{
    const auto eligible = eligible_indices(opers, context);
    std::vector<size_t> result;
    const size_t limit = static_cast<size_t>(std::max(0, context.slots));
    if (limit == 0) {
        return { {}, 0 };
    }

    // 迷迭香在制造站或絮雨在办公室时，优先选择感知信息与无声共鸣体系的宿舍联动干员。
    if (context.use_perception_information &&
        (is_selected(context, "char_391_rosmon") || is_selected(context, "char_436_whispr"))) {
        for (const size_t index : eligible) {
            const auto& oper = opers[index];
            // 琴键漫步 + 慢板行歌：车尔尼；梦境呓语 + 睡前故事：爱丽丝；
            // 无声共鸣 / 无词颂歌：塑心。塑心的稳定角色 ID 检查保留为防御性兼容路径。
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

    // 鸿雪的宿舍联动只选择杜林，或具有大锅饭·α的桃金娘。
    if (is_selected(context, "char_4055_bgsnow")) {
        for (const size_t index : eligible) {
            const auto& oper = opers[index];
            if (has_any_skill(oper, { "bskill_dorm_all&one1", "bskill_dorm_all&one2" }) || // 慵懒 / 嗜睡：杜林
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
            if (icon == "bskill_dorm_all1") { // 鼓舞 / 挣脱：夜莺、赫拉格、四月
                score += 0.1;
            }
            else if (icon == "bskill_dorm_all2") { // 小提琴独奏 / 偶像 / 领袖等：多人共用
                score += 0.15;
            }
            else if (icon == "bskill_dorm_all3") { // 狮心王 / 冬将军 / 提灯女神等：多人共用
                score += 0.2;
            }
            else if (icon == "bskill_dorm_all&one1") { // 慵懒：安比尔、杜林
                score += 0.2;
            }
            else if (icon == "bskill_dorm_all&one2") { // 嗜睡：杜林
                score += 0.25;
            }
            else if (icon == "bskill_dorm_all&one3") { // 牧歌 / 解脱 / “归乡”等：多人共用
                score += 0.1;
            }
            else if (icon == "bskill_hire_spd3") { // 人事管理·β：伊内丝
                score += 0.35;
            }
            else if (icon == "bskill_dorm_powtorecall2") { // 柔和微光·β：流明
                score += 0.15 + context.virtual_power_station_num * 0.05;
            }
            else if (icon == "bskill_dorm_all&single") { // 小酌怡情：冰酿
                score += 0.2;
            }
            else if (icon == "bskill_dorm_all4") { // 芬芳疗养·α：刺玫
                score += 0.15;
            }
            else if (icon == "bskill_dorm_all_tired") { // 芬芳疗养·β：刺玫
                score += 0.151;
            }
            else if (icon == "bskill_dorm_all_tired2") { // 净化呼吸：撷英调香师
                score += 0.09;
            }
            else if (icon == "bskill_dorm_hiretorecall1") { // 寻同路人：斥罪
                score += 0.15;
            }
            else if (icon == "bskill_dorm_hiretorecall2") { // 无瑕心·α：隐德来希
                score += 0.19;
            }
            else if (icon == "bskill_dorm_hiretorecall3") { // 无瑕心·β：隐德来希
                score += 0.29;
            }
            else if (icon == "bskill_dorm_powtorecall1") { // 柔和微光·α：流明
                score += 0.1 + context.virtual_power_station_num * 0.05;
            }
            else if (icon == "bskill_dorm_rec_all&lvl1") { // 睡前必听故事：响石
                score += 0.15;
            }
            else if (icon == "bskill_dorm_rec_all&lv2") { // 死前必做清单：响石
                score += 0.24;
            }
            else if (icon == "bskill_dorm_rec_all&profession") { // 火山温泉浴：纯烬艾雅法拉
                score += 0.06;
            }
            else if (icon == "bskill_dorm_senshi") { // 资深料理人：森西
                score += 0.15;
            }
            else if (icon == "bskill_dorm_unfull") { // 倾谈者：波卜
                score += 0.2;
            }
        }
        return score;
    };
    auto score_single = [](const ScoreOper& oper) {
        double score = 0;
        for (const auto& icon : oper.skills) {
            if (icon == "bskill_dorm_single1") { // 善解人意 / 天生乐天派：多人共用
                score += 0.55;
            }
            else if (icon == "bskill_dorm_single2") { // 医疗服务 / 疗养：波登可、Lancet-2
                score += 0.65;
            }
            else if (icon == "bskill_dorm_single3") { // 维多利亚文学 / 天启 / 心理疏导等：多人共用
                score += 0.7;
            }
            else if (icon == "bskill_dorm_single4") { // 慈悲：闪灵
                score += 0.75;
            }
            else if (icon == "bskill_dorm_single&one01") { // 活泼：米格鲁、卡缇、杰克
                score += 0.2;
            }
            else if (icon == "bskill_dorm_single&one02") { // 探险家的热情：崖心
                score += 0.25;
            }
            else if (icon == "bskill_dorm_single&one21") { // 和谐：泡普卡
                score += 0.4;
            }
            else if (icon == "bskill_dorm_single&one22") { // 使徒 / 喀兰圣女：临光、初雪
                score += 0.5;
            }
            else if (icon == "bskill_dorm_single&one11") { // 烘焙：慕斯
                score += 0.3;
            }
            else if (icon == "bskill_dorm_single&one12") { // 烹饪 / Give me five等：多人共用
                score += 0.35;
            }
            else if (icon == "bskill_dorm_buddy") { // 狩猎好帮手：罗德岛隐秘队
                score += 0.55;
            }
            else if (icon == "bskill_dorm_single_indigo") { // 毒剂师之友：深靛
                score += 0.55;
            }
            else if (icon == "bskill_dorm_single_laterano") { // 圣城趣事通：新约能天使
                score += 0.55;
            }
            else if (icon == "bskill_dorm_single_sami") { // 降生于冰寒：寒檀
                score += 0.55;
            }
            else if (icon == "bskill_dorm_single_schwarz") { // 沏茶：黑
                score += 0.55;
            }
            else if (icon == "bskill_dorm_single_tomimi") { // 烤肉大师：特米米
                score += 0.55;
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

const std::array<AbyssalHunterCandidate, 4>& get_abyssal_hunter_candidates()
{
    // 干员数据缺失时保留空 id，空 id 匹配不到任何干员，等效跳过该候选
    static const auto candidates = [] {
        const auto make_candidate = [](battle::Role role, const char* name) {
            const auto& props = BattleData.find_first_oper(role, name);
            return props ? AbyssalHunterCandidate { props->id, props->role } : AbyssalHunterCandidate {};
        };
        return std::array<AbyssalHunterCandidate, 4> {
            make_candidate(battle::Role::Warrior, "斯卡蒂"),
            make_candidate(battle::Role::Warrior, "幽灵鲨"),
            make_candidate(battle::Role::Warrior, "乌尔比安"),
            make_candidate(battle::Role::Sniper, "安哲拉"),
        };
    }();
    return candidates;
}

bool is_abyssal_hunter(std::string_view operator_id)
{
    return std::ranges::any_of(get_abyssal_hunter_candidates(), [operator_id](const AbyssalHunterCandidate& candidate) {
        return candidate.operator_id == operator_id;
    });
}

void append_abyssal_hunter_candidates(std::vector<ScoreOper>& opers, const ScoreContext& context)
{
    if (!context.use_abyssal_hunter || context.facility != "Mfg" ||
        !context.selected_operator_ids.contains("char_474_glady")) {
        return;
    }

    const auto& candidates = get_abyssal_hunter_candidates();
    opers.reserve(opers.size() + candidates.size());
    for (const auto& candidate : candidates) {
        if (context.selected_operator_ids.contains(candidate.operator_id) ||
            std::ranges::any_of(opers, [&](const ScoreOper& oper) {
                return oper.operator_id == candidate.operator_id;
            })) {
            continue;
        }
        opers.emplace_back(ScoreOper { { std::string(AbyssalHunterSkill) }, candidate.operator_id, "", 1.0 });
    }
}

ScoreResult select_best_opers(const std::vector<ScoreOper>& opers, const ScoreContext& context)
{
    if (context.facility == "Mfg" || context.facility == "Trade") {
        return select_combinations(opers, context);
    }
    if (context.facility == "Office" || context.facility == "Power" || context.facility == "Processing") {
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
    return {};
}

} // namespace asst::infrast
