#include "BattleImageAnalyzer.h"

#include <algorithm>

#include "MultiMatchImageAnalyzer.h"
#include "MatchImageAnalyzer.h"
#include "OcrImageAnalyzer.h"
#include "HashImageAnalyzer.h"
#include "Logger.hpp"
#include "TaskData.h"

bool asst::BattleImageAnalyzer::set_target(int target)
{
    m_target = target;
    return true;
}

bool asst::BattleImageAnalyzer::analyze()
{
    // HP 作为 flag，无论如何都识别。表明当前画面是在战斗场景的
    bool ret = hp_analyze();

    if (m_target & Target::Home) {
        ret |= home_analyze();
    }

    if (!ret) {
        return false;
    }

    // 可能没有干员（全上场了），所以干员识别结果不影响返回值
    if (m_target & Target::Oper) {
        bool oper_ret = opers_analyze();
        if (m_target == Target::Skill) {
            ret = oper_ret;
        }
    }

    // 可能没有可使用的技能，所以技能识别结果不影响返回值
    if (m_target & Target::Skill) {
        bool skill_ret = skill_analyze();
        if (m_target == Target::Skill) {
            ret = skill_ret;
        }
    }

    if (m_target & Target::Kills) {
        ret &= kills_analyze();
    }

    if (m_target & Target::Cost) {
        ret &= cost_analyze();
    }

    if (m_target & Target::Vacancies) {
        ret &= vacancies_analyze();
    }

    return ret;
}

const std::vector<asst::BattleRealTimeOper>& asst::BattleImageAnalyzer::get_opers() const noexcept
{
    return m_opers;
}

const std::vector<asst::Rect>& asst::BattleImageAnalyzer::get_homes() const noexcept
{
    return m_homes;
}

const std::vector<asst::Rect>& asst::BattleImageAnalyzer::get_ready_skills() const noexcept
{
    return m_ready_skills;
}

int asst::BattleImageAnalyzer::get_hp() const noexcept
{
    return m_hp;
}

int asst::BattleImageAnalyzer::get_kills() const noexcept
{
    return m_kills;
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
    const auto cooling_move = Task.get("BattleOperCooling")->rect_move;

    size_t index = 0;
    for (const MatchRect& flag_mrect : flags_analyzer.get_result()) {
        BattleRealTimeOper oper;
        oper.rect = flag_mrect.rect.move(click_move);
        oper.avatar = m_image(utils::make_rect<cv::Rect>(oper.rect));

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

        Rect cooling_rect = flag_mrect.rect.move(cooling_move);
        oper.cooling = oper_cooling_analyze(cooling_rect);
        if (oper.cooling && oper.available) {
            Log.error("oper is available, but with cooling");
        }

        Rect role_rect = flag_mrect.rect.move(role_move);
        oper.role = oper_role_analyze(role_rect);
        Rect cost_rect = flag_mrect.rect.move(cost_move);

        // 费用识别的不太准，暂时也没用上，先注释掉，TODO：优化费用识别
        //oper.cost = oper_cost_analyze(cost_rect);
        oper.index = index++;

        m_opers.emplace_back(std::move(oper));
    }

    return true;
}

asst::BattleRole asst::BattleImageAnalyzer::oper_role_analyze(const Rect& roi)
{
    static const std::unordered_map<BattleRole, std::string> RolesName = {
        { BattleRole::Caster, "Caster" },
        { BattleRole::Medic, "Medic" },
        { BattleRole::Pioneer, "Pioneer" },
        { BattleRole::Sniper, "Sniper" },
        { BattleRole::Special, "Special" },
        { BattleRole::Support, "Support" },
        { BattleRole::Tank, "Tank" },
        { BattleRole::Warrior, "Warrior" },
        { BattleRole::Drone, "Drone" }
    };

    MatchImageAnalyzer role_analyzer(m_image);

    BattleRole result = BattleRole::Unknown;
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

bool asst::BattleImageAnalyzer::oper_cooling_analyze(const Rect& roi)
{
    const auto cooling_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        Task.get("BattleOperCooling"));

    cv::Mat hsv;
    cv::cvtColor(m_image(utils::make_rect<cv::Rect>(roi)), hsv, cv::COLOR_BGR2HSV);
    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);
    int mask_lowb = cooling_task_ptr->mask_range.first;
    int mask_uppb = cooling_task_ptr->mask_range.second;

    int count = 0;
    auto& h_channel = channels.at(0);
    for (int i = 0; i != h_channel.rows; ++i) {
        for (int j = 0; j != h_channel.cols; ++j) {
            cv::uint8_t value = h_channel.at<cv::uint8_t>(i, j);
            if (mask_lowb < value && value < mask_uppb) {
                ++count;
            }
        }
    }
    Log.trace("oper_cooling_analyze |", count);
    return count >= cooling_task_ptr->special_threshold;
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

    static int thres = static_cast<int>(std::dynamic_pointer_cast<MatchTaskInfo>(
        Task.get("BattleOperAvailable"))->special_threshold);
    if (avg[2] < thres) {
        return false;
    }
    return true;
}

bool asst::BattleImageAnalyzer::home_analyze()
{
    // 颜色转换
    cv::Mat hsv;
    cv::cvtColor(m_image, hsv, cv::COLOR_BGR2HSV);
    cv::Mat bin;
    cv::inRange(hsv, cv::Scalar(104, 160, 180), cv::Scalar(107, 220, 255), bin);

    // 开操作降噪
    cv::Mat morph_dst;
    cv::Mat open_kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(1, 1));
    cv::morphologyEx(bin, morph_dst, cv::MORPH_OPEN, open_kernel);

    // 霍夫线
    std::vector<cv::Vec4i> lines;
    cv::HoughLinesP(morph_dst, lines, 1, 60 * CV_PI / 180, 10, 20, 10);

    int left = INT_MAX, right = 0, top = INT_MAX, bottom = 0;
    for (auto&& l : lines) {
        left = (std::min)({ left, l[0], l[2] });
        right = (std::max)({ right, l[0], l[2] });
        top = (std::min)({ top, l[1], l[3] });
        bottom = (std::max)({ bottom, l[1], l[3] });

#ifdef ASST_DEBUG
        cv::line(m_image_draw, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0, 0, 255), 3);
#endif
    }
    if (right == 0) {
        Log.error("home recognition error");
        return false;
    }
    Rect home_rect(left, top, right - left, bottom - top);
    m_homes.emplace_back(home_rect);

#ifdef ASST_DEBUG
    cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(home_rect), cv::Scalar(0, 255, 0), 5);
#endif

    return true;
}

bool asst::BattleImageAnalyzer::skill_analyze()
{
    const auto skill_task_ptr = Task.get("BattleSkillReady");
    const Rect& rect_move = skill_task_ptr->rect_move;

    MultiMatchImageAnalyzer mm_analyzer(m_image);
    mm_analyzer.set_task_info(skill_task_ptr);
    if (!mm_analyzer.analyze()) {
        return false;
    }
    for (const auto& mr : mm_analyzer.get_result()) {
        m_ready_skills.emplace_back(mr.rect.move(rect_move));
    }
    return true;
}

bool asst::BattleImageAnalyzer::hp_analyze()
{
    // 识别 HP 的那个蓝白色图标
    auto flag_task_ptr = Task.get("BattleHpFlag");
    MatchImageAnalyzer flag_analyzer(m_image);
    flag_analyzer.set_task_info(flag_task_ptr);
    if (!flag_analyzer.analyze()) {
        // 漏怪的时候，那个图标会变成红色的，所以多识别一次
        flag_task_ptr = Task.get("BattleHpFlag2");
        flag_analyzer.set_task_info(flag_task_ptr);
        if (!flag_analyzer.analyze()) {
            return false;
        }
    }
    Rect roi_rect = flag_analyzer.get_result().rect.move(flag_task_ptr->rect_move);
    cv::Mat roi = m_image(utils::make_rect<cv::Rect>(roi_rect));

    static const std::array<std::string, 10> NumName = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
    };
    static bool inited = false;
    static cv::Scalar range_lower, range_upper;
    static HashImageAnalyzer hash_analyzer;
    if (!inited) {
        auto [h_l, h_u] = std::dynamic_pointer_cast<HashTaskInfo>(
            Task.get("BattleHpChannelH"))->mask_range;
        auto [s_l, s_u] = std::dynamic_pointer_cast<HashTaskInfo>(
            Task.get("BattleHpChannelS"))->mask_range;
        auto [v_l, v_u] = std::dynamic_pointer_cast<HashTaskInfo>(
            Task.get("BattleHpChannelV"))->mask_range;
        range_lower = cv::Scalar(h_l, s_l, v_l);
        range_upper = cv::Scalar(h_u, s_u, v_u);
        std::unordered_map<std::string, std::string> num_hashs;
        for (auto&& num : NumName) {
            const auto& hashs_vec = std::dynamic_pointer_cast<HashTaskInfo>(
                Task.get("BattleHp" + num))->hashs;
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
    cv::cvtColor(roi, hsv, cv::COLOR_BGR2HSV);
    cv::Mat bin;
    cv::inRange(hsv, range_lower, range_upper, bin);
    hash_analyzer.set_image(bin);
    hash_analyzer.analyze();

    int hp = 0;
    for (const std::string& num_name : hash_analyzer.get_min_dist_name()) {
        if (num_name.empty()) {
            Log.error("hash result is empty");
            return 0;
        }
        hp *= 10;
        hp += num_name.at(0) - '0';
    }

#ifdef ASST_DEBUG
    cv::putText(m_image_draw, "HP: " + std::to_string(hp), cv::Point(roi_rect.x, roi_rect.y + 50), 2, 1, cv::Scalar(0, 0, 255));
#endif
    m_hp = hp;
    return true;
}

bool asst::BattleImageAnalyzer::kills_analyze()
{
    const auto task_ptr = Task.get("BattleKillsFlag");
    MatchImageAnalyzer flag_analzyer(m_image);
    flag_analzyer.set_task_info(task_ptr);
    if (!flag_analzyer.analyze()) {
        return false;
    }

    auto kills_roi = flag_analzyer.get_result().rect.move(task_ptr->rect_move);
    OcrImageAnalyzer kills_analyzer(m_image);
    kills_analyzer.set_roi(kills_roi);
    if (!kills_analyzer.analyze()) {
        return false;
    }
    kills_analyzer.sort_result_by_score();

    std::string kills_text;
    size_t pos = std::string::npos;
    for (auto& res : kills_analyzer.get_result()) {
        // 这里的结果应该是 "击杀数/总的敌人数"，例如 "0/41"
        pos = res.text.find('/');
        if (pos != std::string::npos) {
            kills_text = res.text;
            break;
        }
    }
    if (pos == std::string::npos) {
        return false;
    }

    // 例子中的"0"
    std::string kills_count = kills_text.substr(0, pos);
    if (kills_count.empty() ||
        !std::all_of(kills_count.cbegin(), kills_count.cend(),
            [](char c) -> bool {return std::isdigit(c);})) {
        return false;
    }
    int cur_kills = std::stoi(kills_count);
    m_kills = std::max(m_kills, cur_kills);
    Log.trace("Kills:", m_kills, cur_kills);
    return true;
}

bool asst::BattleImageAnalyzer::cost_analyze()
{
    OcrImageAnalyzer cost_analyzer(m_image);
    cost_analyzer.set_task_info("BattleKillsFlag");

    if (!cost_analyzer.analyze()) {
        return false;
    }

    cost_analyzer.sort_result_by_score();
    std::string cost_str = cost_analyzer.get_result().front().text;

    if (cost_str.empty() ||
    !std::all_of(cost_str.cbegin(), cost_str.cend(),
        [](char c) -> bool {return std::isdigit(c);})) {
        return false;
    }
    m_cost = std::stoi(cost_str);

    return true;
}

bool asst::BattleImageAnalyzer::vacancies_analyze()
{
    return false;
}
