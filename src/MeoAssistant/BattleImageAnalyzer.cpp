#include "BattleImageAnalyzer.h"

#include "MultiMatchImageAnalyzer.h"
#include "MatchImageAnalyzer.h"
#include "HashImageAnalyzer.h"
#include "Logger.hpp"
#include "TaskData.h"

bool asst::BattleImageAnalyzer::analyze()
{
    return opers_analyze();
}

const std::vector<asst::BattleImageAnalyzer::Oper>& asst::BattleImageAnalyzer::get_opers() const noexcept
{
    return m_opers;
}

bool asst::BattleImageAnalyzer::opers_analyze()
{
    LogTraceFunction;

    MultiMatchImageAnalyzer flags_analyzer(m_image);
    flags_analyzer.set_task_info("BattleOpersFlag");
    if (!flags_analyzer.analyze()) {
        return false;
    }
    flags_analyzer.sort_result();

    const auto click_move = Task.get("BattleOperClickRange")->rect_move;
    const auto role_move = Task.get("BattleOperRoleRange")->rect_move;
    const auto cost_move = Task.get("BattleOperCostRange")->rect_move;
    const auto avlb_move = Task.get("BattleOperAvailable")->rect_move;

    for (const MatchRect& flag_mrect : flags_analyzer.get_result()) {
        Oper oper;
        oper.rect = flag_mrect.rect.move(click_move);

        Rect available_rect = flag_mrect.rect.move(avlb_move);
        oper.available = oper_available_analyze(available_rect);

#ifdef ASST_DEBUG
        if (oper.available) {
            cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(oper.rect), cv::Scalar(0, 255, 0), 2);
        }
        else {
            cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(oper.rect), cv::Scalar(0, 0, 255), 2);
        }
#endif

        Rect role_rect = flag_mrect.rect.move(role_move);
        oper.role = oper_role_analyze(role_rect);
        Rect cost_rect = flag_mrect.rect.move(cost_move);

        oper.cost = oper_cost_analyze(cost_rect);

        m_opers.emplace_back(std::move(oper));
    }

    return true;
}

asst::BattleImageAnalyzer::Role asst::BattleImageAnalyzer::oper_role_analyze(const Rect& roi)
{
    static const std::unordered_map<Role, std::string> RolesName = {
        { Role::Caster, "Caster" },
        { Role::Medic, "Medic" },
        { Role::Pioneer, "Pioneer" },
        { Role::Sniper, "Sniper" },
        { Role::Special, "Special" },
        { Role::Support, "Support" },
        { Role::Tank, "Tank" },
        { Role::Warrior, "Warrior" },
        { Role::Drone, "Drone" }
    };

    MatchImageAnalyzer role_analyzer(m_image);

    Role result = Role::Unknown;
    double max_score = 0;
    for (auto&& [role, role_name] : RolesName) {
        role_analyzer.set_task_info("BattleOperRole" + role_name);
        role_analyzer.set_roi(roi);
        if (!role_analyzer.analyze()) {
            continue;
        }
        if (double cur_socre = role_analyzer.get_result().score;
            max_score < cur_socre) {
            result = role;
            max_score = cur_socre;
        }
    }

#ifdef ASST_DEBUG
    std::string role_name;
    if (auto iter = RolesName.find(result);
        iter == RolesName.cend()) {
        role_name = "Unknown";
    }
    else {
        role_name = iter->second;
    }
    cv::putText(m_image_draw, role_name, cv::Point(roi.x, roi.y - 5), 1, 1, cv::Scalar(0, 255, 255));
#endif

    return result;
}

int asst::BattleImageAnalyzer::oper_cost_analyze(const Rect& roi)
{
    static const std::array<std::string, 10> NumName = {
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
    };
    static bool inited = false;
    static cv::Scalar range_lower, range_upper;

    static HashImageAnalyzer hash_analyzer;
    if (!inited) {
        auto [h_l, h_u] = std::dynamic_pointer_cast<HashTaskInfo>(
            Task.get("BattleOperCostChannelH"))->mask_range;
        auto [s_l, s_u] = std::dynamic_pointer_cast<HashTaskInfo>(
            Task.get("BattleOperCostChannelS"))->mask_range;
        auto [v_l, v_u] = std::dynamic_pointer_cast<HashTaskInfo>(
            Task.get("BattleOperCostChannelV"))->mask_range;
        range_lower = cv::Scalar(h_l, s_l, v_l);
        range_upper = cv::Scalar(h_u, s_u, v_u);
        std::unordered_map<std::string, std::string> num_hashs;
        for (auto&& num : NumName) {
            auto hashs_vec = std::dynamic_pointer_cast<HashTaskInfo>(
                Task.get("BattleOperCost" + num))->hashs;
            for (size_t i = 0; i != hashs_vec.size(); ++i) {
                num_hashs.emplace(num + "_" + std::to_string(i), hashs_vec.at(i));
            }
        }
        hash_analyzer.set_hash_templates(std::move(num_hashs));
        hash_analyzer.set_need_bound(true);
        hash_analyzer.set_need_split(true);
        inited = true;
    }

    cv::Mat hsv;
    cv::cvtColor(m_image(utils::make_rect<cv::Rect>(roi)), hsv, cv::COLOR_BGR2HSV);
    cv::Mat bin;
    cv::inRange(hsv, range_lower, range_upper, bin);
    hash_analyzer.set_image(bin);
    hash_analyzer.analyze();

    int cost = 0;
    for (const std::string& num_name : hash_analyzer.get_min_dist_name()) {
        if (num_name.empty()) {
            Log.error("hash result is empty");
            return 0;
        }
        cost *= 10;
        cost += num_name.at(0) - '0';
    }

#ifdef ASST_DEBUG
    cv::putText(m_image_draw, std::to_string(cost), cv::Point(roi.x, roi.y - 20), 1, 1, cv::Scalar(0, 0, 255));
#endif

    return cost;
}

bool asst::BattleImageAnalyzer::oper_available_analyze(const Rect& roi)
{
    cv::Mat hsv;
    cv::cvtColor(m_image(utils::make_rect<cv::Rect>(roi)), hsv, cv::COLOR_BGR2HSV);
    cv::Scalar avg = cv::mean(hsv);
    Log.trace("oper available, mean", avg[2]);

    static int thres = std::dynamic_pointer_cast<MatchTaskInfo>(Task.get("BattleOperAvailable"))->special_threshold;
    if (avg[2] < thres) {
        return false;
    }
    return true;
}

bool asst::BattleImageAnalyzer::home_analyze()
{
    return false;
}

bool asst::BattleImageAnalyzer::placed_analyze()
{
    return false;
}
