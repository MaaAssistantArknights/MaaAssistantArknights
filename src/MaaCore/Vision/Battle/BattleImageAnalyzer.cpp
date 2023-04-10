#include "BattleImageAnalyzer.h"

#include "Utils/Ranges.hpp"
#include <algorithm>

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"
#include "Vision/BestMatchImageAnalyzer.h"
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

    // flag 无论如何都识别。表明当前画面是在战斗场景的
    bool ret = flag_analyze();
    if (!ret) {
        return false;
    }

    //
    // if (m_target & Target::Home) {
    //    ret |= home_analyze();
    //}

    // 可能没有干员（全上场了），所以干员识别结果不影响返回值
    if (m_target & Target::Oper) {
        opers_analyze();
    }

    if (m_target & Target::DetailPage) {
        detail_page_analyze();
    }

    if (m_target & Target::Kills) {
        ret &= kills_analyze();
    }

    if (m_target & Target::Cost) {
        ret &= cost_analyze();
    }

    // if (m_target & Target::Vacancies) {
    //     ret &= vacancies_analyze();
    // }

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

bool asst::BattleImageAnalyzer::get_in_detail_page() const noexcept
{
    return m_in_detail_page;
}

bool asst::BattleImageAnalyzer::get_pause_button() const noexcept
{
    return m_pause_button;
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
    MultiMatchImageAnalyzer flags_analyzer(m_image);
    const auto& flag_task_ptr = Task.get("BattleOpersFlag");
    flags_analyzer.set_task_info(flag_task_ptr);
#ifndef ASST_DEBUG
    flags_analyzer.set_log_tracing(false);
#endif
    if (!flags_analyzer.analyze()) {
        return false;
    }
    flags_analyzer.sort_result_horizontal();
    auto flags = flags_analyzer.get_result();
    if (flags.empty()) {
        return false;
    }

    const auto click_move = Task.get("BattleOperClickRange")->rect_move;
    const auto role_move = Task.get("BattleOperRoleRange")->rect_move;
    // const auto cost_move = Task.get("BattleOperCostRange")->rect_move;
    const auto avlb_move = Task.get("BattleOperAvailable")->rect_move;
    const auto cooling_move = Task.get("BattleOperCooling")->rect_move;
    const auto avatar_move = Task.get("BattleOperAvatar")->rect_move;
    // const int unselected_y = flag_task_ptr->roi.y;

    size_t index = 0;
    for (const MatchRect& flag_mrect : flags) {
        battle::DeploymentOper oper;
        oper.rect = flag_mrect.rect.move(click_move);

        Rect role_rect = flag_mrect.rect.move(role_move);
        oper.role = oper_role_analyze(role_rect);
        if (oper.role == battle::Role::Unknown) {
            Log.warn("Unknown role");
            continue;
        }

        if (oper.rect.x + oper.rect.width >= m_image.cols) {
            oper.rect.width = m_image.cols - oper.rect.x;
        }
        Rect avatar_rect = oper.rect.move(avatar_move);
        oper.avatar = m_image(make_rect<cv::Rect>(avatar_rect));

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
        if (cooling_rect.y + cooling_rect.height >= m_image.rows) {
            cooling_rect.height = m_image.rows - cooling_rect.y;
        }
        oper.cooling = oper_cooling_analyze(cooling_rect);
        if (oper.cooling && oper.available) {
            Log.error("oper is available, but with cooling");
        }

        // 干员费用识别的不太准，暂时也没用上，先注释掉，TODO：优化费用识别
        // Rect cost_rect = flag_mrect.rect.move(cost_move);
        // oper.cost = oper_cost_analyze(cost_rect);
        oper.index = index++;
        // oper.selected = flag_mrect.rect.y < unselected_y;

        m_opers.emplace_back(std::move(oper));
    }

    return true;
}

asst::battle::Role asst::BattleImageAnalyzer::oper_role_analyze(const Rect& roi)
{
    static const std::unordered_map<std::string, battle::Role> RoleMap = {
        { "Caster", battle::Role::Caster }, { "Medic", battle::Role::Medic },     { "Pioneer", battle::Role::Pioneer },
        { "Sniper", battle::Role::Sniper }, { "Special", battle::Role::Special }, { "Support", battle::Role::Support },
        { "Tank", battle::Role::Tank },     { "Warrior", battle::Role::Warrior }, { "Drone", battle::Role::Drone },
    };

    static const std::string TaskName = "BattleOperRole";
    static const std::string Ext = ".png";
    BestMatchImageAnalyzer role_analyzer(m_image);
#ifndef ASST_DEBUG
    role_analyzer.set_log_tracing(false);
#endif // !ASST_DEBUG
    role_analyzer.set_task_info(TaskName);
    role_analyzer.set_roi(roi);

    for (const auto& role_name : RoleMap | views::keys) {
        role_analyzer.append_templ(TaskName + role_name + Ext);
    }
    if (!role_analyzer.analyze()) {
        Log.warn(__FUNCTION__, "unknown role");
        return battle::Role::Unknown;
    }

    const auto& templ_name = role_analyzer.get_result().name;

    std::string role_name = templ_name.substr(TaskName.size(), templ_name.size() - TaskName.size() - Ext.size());

#ifdef ASST_DEBUG
    cv::putText(m_image_draw, role_name, cv::Point(roi.x, roi.y - 5), 1, 1, cv::Scalar(0, 255, 255));
#endif

    return RoleMap.at(role_name);
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
    // Log.trace("oper_cooling_analyze |", count);
    return count >= cooling_task_ptr->special_params.front();
}

int asst::BattleImageAnalyzer::oper_cost_analyze(const Rect& roi)
{
    std::ignore = roi;
    return 0;
}

bool asst::BattleImageAnalyzer::oper_available_analyze(const Rect& roi)
{
    cv::Mat hsv;
    cv::cvtColor(m_image(make_rect<cv::Rect>(roi)), hsv, cv::COLOR_BGR2HSV);
    cv::Scalar avg = cv::mean(hsv);
    // Log.trace("oper available, mean", avg[2]);

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

bool asst::BattleImageAnalyzer::hp_flag_analyze()
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
    return true;
}

bool asst::BattleImageAnalyzer::kills_flag_analyze()
{
    MatchImageAnalyzer flag_analyzer(m_image);
    flag_analyzer.set_task_info("BattleKillsFlag");
    return flag_analyzer.analyze();
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
        if (m_pre_total_kills <= 0) {
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
    return m_kills <= m_total_kills;
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
    return pause_button_analyze() || hp_flag_analyze() || kills_flag_analyze();
}

bool asst::BattleImageAnalyzer::pause_button_analyze()
{
    auto has_started_task_ptr = Task.get("BattleHasStarted");
    cv::Mat roi = m_image(make_rect<cv::Rect>(has_started_task_ptr->roi));
    cv::Mat roi_gray;
    cv::cvtColor(roi, roi_gray, cv::COLOR_BGR2GRAY);
    cv::Mat bin;
    const int value_threshold = has_started_task_ptr->special_params[0];
    cv::threshold(roi_gray, bin, value_threshold, 255, cv::THRESH_BINARY);
    int count = cv::countNonZero(bin);
    const int count_threshold = has_started_task_ptr->special_params[1];
    Log.trace(__FUNCTION__, "count", count, "threshold", count_threshold);

    m_pause_button = count > count_threshold;
    return m_pause_button;
}

bool asst::BattleImageAnalyzer::detail_page_analyze()
{
    auto analyze = [&](const std::string& task_name) {
        auto task_ptr = Task.get(task_name);
        cv::Mat roi = m_image(make_rect<cv::Rect>(task_ptr->roi));
        cv::Mat roi_hsv;
        cv::cvtColor(roi, roi_hsv, cv::COLOR_BGR2HSV);
        cv::Mat bin1;
        cv::inRange(roi_hsv, cv::Scalar(99, 235, 235), cv::Scalar(105, 255, 255), bin1);
        int count1 = cv::countNonZero(bin1);

        cv::Mat bin2;
        cv::inRange(roi_hsv, cv::Scalar(99, 235, 135), cv::Scalar(105, 255, 155), bin2);
        int count2 = cv::countNonZero(bin2);

        const int threshold = task_ptr->special_params[0];
        Log.info("detail_page, count:", count1, count2, ", threshold:", threshold);

        return count1 > threshold || count2 > threshold;
    };

    m_in_detail_page = analyze("BattleOperDetailPageFlag") || analyze("BattleOperDetailPageOldFlag");
    return m_in_detail_page;
}
