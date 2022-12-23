#include "BattleImageAnalyzer.h"

#include "Utils/Ranges.hpp"
#include <algorithm>

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"
#include "Vision/HashImageAnalyzer.h"
#include "Vision/MatchImageAnalyzer.h"
#include "Vision/MultiMatchImageAnalyzer.h"
#include "Vision/OcrWithFlagTemplImageAnalyzer.h"

bool asst::BattleImageAnalyzer::set_target(int target)
{
    m_target = target;
    return true;
}

void asst::BattleImageAnalyzer::set_pre_total_kills(int pre_total_kills)
{
    m_pre_total_kills = pre_total_kills;
}

bool asst::BattleImageAnalyzer::analyze()
{
    clear();

    // HP 作为 flag，无论如何都识别。表明当前画面是在战斗场景的
    bool ret = hp_analyze() || flag_analyze();

    if (m_target & Target::Home) {
        ret |= home_analyze();
    }

    if (!ret) {
        return false;
    }

    // 可能没有干员（全上场了），所以干员识别结果不影响返回值
    if (m_target & Target::Oper) {
        opers_analyze();
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

const std::vector<asst::battle::DeploymentOper>& asst::BattleImageAnalyzer::get_opers() const noexcept
{
    return m_opers;
}

const std::vector<asst::Rect>& asst::BattleImageAnalyzer::get_homes() const noexcept
{
    return m_homes;
}

int asst::BattleImageAnalyzer::get_hp() const noexcept
{
    return m_hp;
}

int asst::BattleImageAnalyzer::get_kills() const noexcept
{
    return m_kills;
}

int asst::BattleImageAnalyzer::get_total_kills() const noexcept
{
    return m_total_kills;
}

int asst::BattleImageAnalyzer::get_cost() const noexcept
{
    return m_cost;
}

void asst::BattleImageAnalyzer::clear() noexcept
{
    m_opers.clear();
    m_homes.clear();
    m_ready_skills.clear();
    m_hp = 0;
    m_kills = 0;
    m_cost = 0;
}

void asst::BattleImageAnalyzer::sort_opers_by_cost()
{
    // 本来游戏就是按费用排的，这里倒序一下就行了
    ranges::reverse(m_opers);
}

bool asst::BattleImageAnalyzer::opers_analyze()
{
    LogTraceFunction;

    MultiMatchImageAnalyzer flags_analyzer(m_image);
    flags_analyzer.set_task_info("BattleOpersFlag");
    if (!flags_analyzer.analyze()) {
        return false;
    }
    flags_analyzer.sort_result_horizontal();

    const auto click_move = Task.get("BattleOperClickRange")->rect_move;
    const auto role_move = Task.get("BattleOperRoleRange")->rect_move;
    // const auto cost_move = Task.get("BattleOperCostRange")->rect_move;
    const auto avlb_move = Task.get("BattleOperAvailable")->rect_move;
    const auto cooling_move = Task.get("BattleOperCooling")->rect_move;

    size_t index = 0;
    for (const MatchRect& flag_mrect : flags_analyzer.get_result()) {
        battle::DeploymentOper oper;
        oper.rect = flag_mrect.rect.move(click_move);
        if (oper.rect.x + oper.rect.width >= m_image.cols) {
            oper.rect.width = m_image.cols - oper.rect.x;
        }
        auto avatar_rect = make_rect<cv::Rect>(oper.rect);
        avatar_rect.y += 30;
        avatar_rect.height = 70;
        oper.avatar = m_image(avatar_rect);

        Rect available_rect = flag_mrect.rect.move(avlb_move);
        oper.available = oper_available_analyze(available_rect);

#ifdef ASST_DEBUG
        if (oper.available) {
            cv::rectangle(m_image_draw, make_rect<cv::Rect>(oper.rect), cv::Scalar(0, 255, 0), 2);
        }
        else {
            cv::rectangle(m_image_draw, make_rect<cv::Rect>(oper.rect), cv::Scalar(0, 0, 255), 2);
        }
#endif

        Rect cooling_rect = flag_mrect.rect.move(cooling_move);
        if (cooling_rect.x + cooling_rect.width >= m_image.cols) {
            cooling_rect.width = m_image.cols - cooling_rect.x;
        }
        oper.cooling = oper_cooling_analyze(cooling_rect);
        if (oper.cooling && oper.available) {
            Log.error("oper is available, but with cooling");
        }

        Rect role_rect = flag_mrect.rect.move(role_move);
        oper.role = oper_role_analyze(role_rect);

        // 费用识别的不太准，暂时也没用上，先注释掉，TODO：优化费用识别
        // Rect cost_rect = flag_mrect.rect.move(cost_move);
        // oper.cost = oper_cost_analyze(cost_rect);
        oper.index = index++;

        m_opers.emplace_back(std::move(oper));
    }

    return true;
}

asst::battle::Role asst::BattleImageAnalyzer::oper_role_analyze(const Rect& roi)
{
    static const std::unordered_map<battle::Role, std::string> RolesName = {
        { battle::Role::Caster, "Caster" }, { battle::Role::Medic, "Medic" },     { battle::Role::Pioneer, "Pioneer" },
        { battle::Role::Sniper, "Sniper" }, { battle::Role::Special, "Special" }, { battle::Role::Support, "Support" },
        { battle::Role::Tank, "Tank" },     { battle::Role::Warrior, "Warrior" }, { battle::Role::Drone, "Drone" }
    };

    MatchImageAnalyzer role_analyzer(m_image);

    auto result = battle::Role::Unknown;
    double max_score = 0;
    for (auto&& [role, role_name] : RolesName) {
        role_analyzer.set_task_info("BattleOperRole" + role_name);
        role_analyzer.set_roi(roi);
        if (!role_analyzer.analyze()) {
            continue;
        }
        if (double cur_score = role_analyzer.get_result().score; max_score < cur_score) {
            result = role;
            max_score = cur_score;
        }
    }

#ifdef ASST_DEBUG
    std::string role_name;
    if (auto iter = RolesName.find(result); iter == RolesName.cend()) {
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
    const auto cooling_task_ptr = Task.get<MatchTaskInfo>("BattleOperCooling");

    auto img_roi = m_image(make_rect<cv::Rect>(roi));
    cv::Mat hsv;
    cv::cvtColor(img_roi, hsv, cv::COLOR_BGR2HSV);
    int h_low = cooling_task_ptr->mask_range.first;
    int h_up = cooling_task_ptr->mask_range.second;
    int s_low = cooling_task_ptr->specific_rect.x;
    int s_up = cooling_task_ptr->specific_rect.y;
    int v_low = cooling_task_ptr->specific_rect.width;
    int v_up = cooling_task_ptr->specific_rect.height;

    cv::Mat bin;
    cv::inRange(hsv, cv::Scalar(h_low, s_low, v_low), cv::Scalar(h_up, s_up, v_up), bin);

    int count = 0;
    for (int i = 0; i != bin.rows; ++i) {
        for (int j = 0; j != bin.cols; ++j) {
            cv::uint8_t value = bin.at<cv::uint8_t>(i, j);
            if (value) {
                ++count;
            }
        }
    }
    Log.trace("oper_cooling_analyze |", count);
    return count >= cooling_task_ptr->special_params.front();
}

int asst::BattleImageAnalyzer::oper_cost_analyze(const Rect& roi)
{
    static const std::array<std::string, 10> NumName = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
    static bool inited = false;
    static cv::Scalar range_lower, range_upper;

    static HashImageAnalyzer hash_analyzer;
    if (!inited) {
        auto [h_l, h_u] = std::dynamic_pointer_cast<HashTaskInfo>(Task.get("BattleOperCostChannelH"))->mask_range;
        auto [s_l, s_u] = std::dynamic_pointer_cast<HashTaskInfo>(Task.get("BattleOperCostChannelS"))->mask_range;
        auto [v_l, v_u] = std::dynamic_pointer_cast<HashTaskInfo>(Task.get("BattleOperCostChannelV"))->mask_range;
        range_lower = cv::Scalar(h_l, s_l, v_l);
        range_upper = cv::Scalar(h_u, s_u, v_u);
        std::unordered_map<std::string, std::string> num_hashes;
        for (auto&& num : NumName) {
            auto hashes_vec = std::dynamic_pointer_cast<HashTaskInfo>(Task.get("BattleOperCost" + num))->hashes;
            for (size_t i = 0; i != hashes_vec.size(); ++i) {
                num_hashes.emplace(num + "_" + std::to_string(i), hashes_vec.at(i));
            }
        }
        hash_analyzer.set_hash_templates(std::move(num_hashes));
        hash_analyzer.set_need_bound(true);
        hash_analyzer.set_need_split(true);
        inited = true;
    }

    cv::Mat hsv;
    cv::cvtColor(m_image(make_rect<cv::Rect>(roi)), hsv, cv::COLOR_BGR2HSV);
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
    cv::cvtColor(m_image(make_rect<cv::Rect>(roi)), hsv, cv::COLOR_BGR2HSV);
    cv::Scalar avg = cv::mean(hsv);
    Log.trace("oper available, mean", avg[2]);

    const int thres = Task.get("BattleOperAvailable")->special_params.front();
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
    cv::rectangle(m_image_draw, make_rect<cv::Rect>(home_rect), cv::Scalar(0, 255, 0), 5);
#endif

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
    cv::Mat roi = m_image(make_rect<cv::Rect>(roi_rect));

    static const std::array<std::string, 10> NumName = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
    static bool inited = false;
    static cv::Scalar range_lower, range_upper;
    static HashImageAnalyzer hash_analyzer;
    if (!inited) {
        auto [h_l, h_u] = std::dynamic_pointer_cast<HashTaskInfo>(Task.get("BattleHpChannelH"))->mask_range;
        auto [s_l, s_u] = std::dynamic_pointer_cast<HashTaskInfo>(Task.get("BattleHpChannelS"))->mask_range;
        auto [v_l, v_u] = std::dynamic_pointer_cast<HashTaskInfo>(Task.get("BattleHpChannelV"))->mask_range;
        range_lower = cv::Scalar(h_l, s_l, v_l);
        range_upper = cv::Scalar(h_u, s_u, v_u);
        std::unordered_map<std::string, std::string> num_hashes;
        for (auto&& num : NumName) {
            const auto& hashes_vec = std::dynamic_pointer_cast<HashTaskInfo>(Task.get("BattleHp" + num))->hashes;
            for (size_t i = 0; i != hashes_vec.size(); ++i) {
                num_hashes.emplace(num + "_" + std::to_string(i), hashes_vec.at(i));
            }
        }
        hash_analyzer.set_hash_templates(std::move(num_hashes));
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
            return false;
        }
        hp *= 10;
        hp += num_name.at(0) - '0';
    }

#ifdef ASST_DEBUG
    cv::putText(m_image_draw, "HP: " + std::to_string(hp), cv::Point(roi_rect.x, roi_rect.y + 50), 2, 1,
                cv::Scalar(0, 0, 255));
#endif
    m_hp = hp;
    return true;
}

bool asst::BattleImageAnalyzer::kills_analyze()
{
    OcrWithFlagTemplImageAnalyzer kills_analyzer(m_image);
    kills_analyzer.set_task_info("BattleKillsFlag", "BattleKills");
    kills_analyzer.set_replace(Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map);

    if (!kills_analyzer.analyze()) {
        return false;
    }

    std::string kills_text = kills_analyzer.get_result().front().text;
    if (kills_text.empty()) {
        return false;
    }

    size_t pos = kills_text.find('/');
    if (pos == std::string::npos) {
        Log.warn("cannot found flag /");
        // 这种时候绝大多数是把 "0/41" 中的 '/' 识别成了别的什么东西（其中又有绝大部分情况是识别成了 '1'）
        // 所以这里依赖 m_pre_total_kills 转一下
        if (m_pre_total_kills == 0) {
            // 第一次识别就识别错了，识别成了 "0141"
            if (kills_text.at(0) == '0') {
                pos = 1;
            }
            else {
                Log.error("m_pre_total_kills is zero");
                return false;
            }
        }
        else {
            size_t pre_pos = kills_text.find(std::to_string(m_pre_total_kills));
            if (pre_pos == std::string::npos || pre_pos == 0) {
                Log.error("can't get pre_pos");
                return false;
            }
            Log.trace("pre total kills pos:", pre_pos);
            pos = pre_pos - 1;
        }
    }

    // 例子中的"0"
    std::string kills_count = kills_text.substr(0, pos);
    if (kills_count.empty() || !ranges::all_of(kills_count, [](char c) -> bool { return std::isdigit(c); })) {
        return false;
    }
    int cur_kills = std::stoi(kills_count);
    m_kills = std::max(m_kills, cur_kills);

    // 例子中的"41"
    std::string total_kills = kills_text.substr(pos + 1, std::string::npos);
    int cur_total_kills = 0;
    if (total_kills.empty() || !ranges::all_of(total_kills, [](char c) -> bool { return std::isdigit(c); })) {
        Log.warn("total kills recognition failed, set to", m_pre_total_kills);
        cur_total_kills = m_pre_total_kills;
    }
    else {
        cur_total_kills = std::stoi(total_kills);
    }
    m_total_kills = std::max(cur_total_kills, m_pre_total_kills);

    Log.trace("Kills:", m_kills, "/", m_total_kills);
    return true;
}

bool asst::BattleImageAnalyzer::cost_analyze()
{
    OcrWithPreprocessImageAnalyzer cost_analyzer(m_image);
    cost_analyzer.set_task_info("BattleCostData");
    cost_analyzer.set_replace(Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map);

    if (!cost_analyzer.analyze()) {
        return false;
    }

    std::string cost_str = cost_analyzer.get_result().front().text;

    if (cost_str.empty() || !ranges::all_of(cost_str, [](const char& c) -> bool { return std::isdigit(c); })) {
        return false;
    }
    m_cost = std::stoi(cost_str);

    return true;
}

bool asst::BattleImageAnalyzer::vacancies_analyze()
{
    return false;
}

bool asst::BattleImageAnalyzer::flag_analyze()
{
    MatchImageAnalyzer flag_analyzer(m_image);
    flag_analyzer.set_task_info("BattleOfficiallyBegin");
    return flag_analyzer.analyze();
}
