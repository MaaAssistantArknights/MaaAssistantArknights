#include "InfrastProductionTask.h"

#include <algorithm>
#include <ranges>

#include <calculator/calculator.hpp>

#include "Config/Miscellaneous/InfrastConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Hasher.h"
#include "Vision/Infrast/InfrastOperImageAnalyzer.h"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

asst::InfrastProductionTask& asst::InfrastProductionTask::set_uses_of_drone(std::string uses_of_drones) noexcept
{
    m_uses_of_drones = std::move(uses_of_drones);
    return *this;
}

std::string asst::InfrastProductionTask::get_uses_of_drone() const noexcept
{
    return m_uses_of_drones;
}

void asst::InfrastProductionTask::set_custom_drones_config(infrast::CustomDronesConfig drones_config)
{
    m_is_use_custom_drones = true;
    m_custom_drones_config = std::move(drones_config);
}

void asst::InfrastProductionTask::clear_custom_drones_config()
{
    m_is_use_custom_drones = false;
}

void asst::InfrastProductionTask::set_product(std::string product_name) noexcept
{
    m_product = std::move(product_name);

    {
        json::value callback_info = basic_info_with_what("ProductOfFacility");
        callback_info["details"]["product"] = m_product;
        // 该回调注册了插件 DronesForShamareTaskPlugin
        callback(AsstMsg::SubTaskExtraInfo, callback_info);
    }

    if (m_is_custom && current_room_config().product != infrast::CustomRoomConfig::Product::Unknown) {
        static const std::unordered_map<std::string, infrast::CustomRoomConfig::Product> ProductMap = {
            { "CombatRecord", infrast::CustomRoomConfig::Product::BattleRecord },
            { "PureGold", infrast::CustomRoomConfig::Product::PureGold },
            { "OriginStone", infrast::CustomRoomConfig::Product::OriginiumShard },
            { "Chip", infrast::CustomRoomConfig::Product::Dualchip },
            { "Money", infrast::CustomRoomConfig::Product::LMD },
            { "SyntheticJade", infrast::CustomRoomConfig::Product::Orundum },
        };
        if (auto iter = ProductMap.find(m_product); iter != ProductMap.cend()) {
            bool is_same_product = iter->second == current_room_config().product;
            if (!is_same_product) {
                json::value callback_info = basic_info_with_what("ProductIncorrect");
                callback_info["details"]["product"] = m_product;
                callback(AsstMsg::SubTaskExtraInfo, callback_info);
            }
            m_is_product_incorrect = !is_same_product;
        }
    }
}

void asst::InfrastProductionTask::change_product()
{
    auto customProduct = current_room_config().product;
    switch (customProduct) {
    /*制造站的产品类型*/
    case infrast::CustomRoomConfig::Product::BattleRecord: {
        ProcessTask(*this, { "ChangeProductToMiddleBattleRecord" }).run();
        json::value callback_info = basic_info_with_what("ProductChanged");
        callback_info["details"]["product"] = "MiddleBattleRecord";
        callback(AsstMsg::SubTaskExtraInfo, callback_info);
        break;
    }
    case infrast::CustomRoomConfig::Product::PureGold: {
        ProcessTask(*this, { "ChangeProductToPureGold" }).run();
        json::value callback_info = basic_info_with_what("ProductChanged");
        callback_info["details"]["product"] = "PureGold";
        callback(AsstMsg::SubTaskExtraInfo, callback_info);
        break;
    }
    case infrast::CustomRoomConfig::Product::OriginiumShard: {
        ProcessTask(*this, { "ChangeProductToOriginiumShard" }).run();
        json::value callback_info = basic_info_with_what("ProductChanged");
        callback_info["details"]["product"] = "OriginiumShard";
        callback(AsstMsg::SubTaskExtraInfo, callback_info);
        break;
    }
    case infrast::CustomRoomConfig::Product::Dualchip: {
        break;
    }
    /*贸易站的订单类型*/
    case infrast::CustomRoomConfig::Product::LMD: {
        ProcessTask(*this, { "ChangeToMoneyOrder" }).run();
        json::value callback_info = basic_info_with_what("ProductChanged");
        callback_info["details"]["product"] = "Money";
        callback(AsstMsg::SubTaskExtraInfo, callback_info);
        break;
    }
    case infrast::CustomRoomConfig::Product::Orundum: {
        ProcessTask(*this, { "ChangeToSyntheticJadeFlagOrder" }).run();
        json::value callback_info = basic_info_with_what("ProductChanged");
        callback_info["details"]["product"] = "SyntheticJade";
        callback(AsstMsg::SubTaskExtraInfo, callback_info);
        break;
    }

    default: {
        break;
    }
    }
}

bool asst::InfrastProductionTask::shift_facility_list()
{
    LogTraceFunction;
    if (!facility_list_detect() || need_exit()) {
        return false;
    }
    const auto tab_task_ptr = Task.get("InfrastFacilityListTab" + facility_name());

    for (; static_cast<size_t>(m_cur_facility_index) < m_facility_list_tabs.size(); ++m_cur_facility_index) {
        Rect tab = m_facility_list_tabs.at(m_cur_facility_index);
        if (need_exit()) {
            return false;
        }
        if (m_cur_facility_index != 0) {
            callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("EnterFacility"));
            if (m_is_custom) {
                if (static_cast<size_t>(m_cur_facility_index) < m_custom_config.size()) {
                    current_room_config() = m_custom_config.at(m_cur_facility_index);
                }
                else {
                    Log.warn("index out of range:", m_cur_facility_index, m_custom_config.size());
                    break;
                }
            }
        }

        // 最近总是出现设施没选中，导致干员放错房间，多点一次，观察一段时间。最好是改为图像识别是几号。
        sleep(tab_task_ptr->pre_delay);
        ctrler()->click(tab);
        sleep(tab_task_ptr->post_delay);

        sleep(tab_task_ptr->pre_delay);
        ctrler()->click(tab);
        sleep(tab_task_ptr->post_delay);

        /* 识别当前制造/贸易站有没有添加干员按钮，没有就不换班 */
        const auto image = ctrler()->get_image();
        Matcher add_analyzer(image);
        const auto add_task_ptr = Task.get("InfrastAddOperator" + facility_name() + m_work_mode_name);
        add_analyzer.set_task_info(add_task_ptr);
        if (!add_analyzer.analyze()) {
            Log.error("no add button, just continue");
            continue;
        }
        Rect add_button = add_analyzer.get_result().rect;
        auto& rect_move = add_task_ptr->rect_move;
        if (!rect_move.empty()) {
            add_button = add_button.move(rect_move);
        }

        /* 识别当前正在造什么 */
        Matcher product_analyzer(image);
        auto& all_products = InfrastData.get_facility_info(facility_name()).products;
        std::string cur_product = all_products.at(0);
        double max_score = 0;
        for (const std::string& product : all_products) {
            product_analyzer.set_task_info("InfrastFlag" + product);
            if (product_analyzer.analyze()) {
                double score = product_analyzer.get_result().score;
                if (score > max_score) {
                    max_score = score;
                    cur_product = product;
                }
            }
        }
        set_product(cur_product);

        MultiMatcher locked_analyzer(image);
        locked_analyzer.set_task_info("InfrastOperLocked" + facility_name());
        if (locked_analyzer.analyze()) {
            m_cur_num_of_locked_opers = static_cast<int>(locked_analyzer.get_result().size());
        }
        else {
            m_cur_num_of_locked_opers = 0;
        }

        // 使用无人机
        if (m_is_use_custom_drones && m_custom_drones_config.order == infrast::CustomDronesConfig::Order::Pre &&
            m_custom_drones_config.index == m_cur_facility_index) {
            use_drone();
        }

        if (m_is_custom && current_room_config().skip) {
            Log.info("skip this room");
            continue;
        }

        /* 进入干员选择页面 */
        if (!m_skip_shift) {
            ctrler()->click(add_button);
            sleep(add_task_ptr->post_delay);

            close_quick_formation_expand_role();

            // 如果是使用了编队组来排班
            if (current_room_config().use_operator_groups) {
                match_operator_groups();
            }

            for (int i = 0; i <= OperSelectRetryTimes; ++i) {
                if (need_exit()) {
                    return false;
                }

                if (is_use_custom_opers()) {
                    bool name_select_ret = swipe_and_select_custom_opers();
                    if (name_select_ret) {
                        break;
                    }
                    else {
                        swipe_to_the_left_of_operlist();
                        continue;
                    }
                }

                if (m_all_available_opers.empty()) {
                    if (!opers_detect_with_swipe()) {
                        return false;
                    }
                    swipe_to_the_left_of_operlist();
                }
                else {
                    opers_detect();
                }

                optimal_calc();

                // 清空按钮放到识别完之后，现在通过切换职业栏来回到界面最左侧，先清空会导致当前设施里的人排到最后面
                click_clear_button();
                if (!opers_choose()) {
                    m_all_available_opers.clear();
                    swipe_to_the_left_of_operlist();
                    continue;
                }
                break;
            }
            click_confirm_button();
        }
        else {
            Log.info("skip shift in rotation mode");
        }

        /*启用自定义基建时，如果产物不一致则直接更换产物*/
        if (m_is_custom && m_is_product_incorrect) {
            change_product();
        }
        // 使用无人机
        if (m_is_use_custom_drones) {
            if (m_custom_drones_config.order == infrast::CustomDronesConfig::Order::Post &&
                m_custom_drones_config.index == m_cur_facility_index) {
                use_drone();
            }
        }
        else if (cur_product == m_uses_of_drones) {
            if (use_drone()) {
                m_uses_of_drones = "_Used";
            }
        }
    }
    return true;
}

bool asst::InfrastProductionTask::opers_detect_with_swipe()
{
    LogTraceFunction;
    m_all_available_opers.clear();

    while (true) {
        if (need_exit()) {
            return false;
        }
        size_t num = opers_detect();
        Log.trace("opers_detect return", num);

        if (num == 0) {
            break;
        }

        // 异步在最后会多滑动一下，耽误的时间还不如用同步
        swipe_of_operlist();
    }

    if (!m_all_available_opers.empty()) {
        return true;
    }
    return false;
}

size_t asst::InfrastProductionTask::opers_detect()
{
    LogTraceFunction;
    const auto image = ctrler()->get_image();

    InfrastOperImageAnalyzer oper_analyzer(image);
    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");
    oper_analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::All);
    oper_analyzer.set_facility(facility_name());

    if (!oper_analyzer.analyze()) {
        return 0;
    }
    const auto& cur_all_opers = oper_analyzer.get_result();
    max_num_of_opers_per_page = (std::max)(max_num_of_opers_per_page, cur_all_opers.size());

    const int face_hash_thres = Task.get("InfrastOperFace")->special_params[0];
    const size_t pre_size = m_all_available_opers.size();
    for (const auto& cur_oper : cur_all_opers) {
        if (cur_oper.skills.empty()) {
            continue;
        }
        // 在黑名单中的干员不可用
        if (m_is_custom && !current_room_config().blacklist.empty()) {
            const auto& blacklist = current_room_config().blacklist;
            RegionOCRer name_analyzer;
            name_analyzer.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
            name_analyzer.set_image(cur_oper.name_img);
            name_analyzer.set_bin_expansion(0);
            if (!name_analyzer.analyze()) {
                Log.trace("ERROR:!name_analyzer.analyze()");
                continue;
            }
            const std::string& name = name_analyzer.get_result().text;
            if (std::any_of(blacklist.begin(), blacklist.end(), [&](const std::string& s) { return s == name; })) {
                Log.trace("Skip operator", name, "in blacklist");
                continue;
            }
        }
        {
            std::string skills_str = "[";
            for (const auto& skill : cur_oper.skills) {
                skills_str += skill.id + ", ";
            }
            skills_str += "]";
            Log.trace(skills_str, "mood", cur_oper.mood_ratio, "threshold", m_mood_threshold);
        }
        // 心情过低的干员则不可用
        if (cur_oper.mood_ratio < m_mood_threshold) {
            //--cur_available_num;
            continue;
        }
        auto find_iter = std::ranges::find_if(m_all_available_opers, [&](const infrast::Oper& oper) -> bool {
            if (oper.skills != cur_oper.skills) {
                return false;
            }
            // 有可能是同一个干员，比一下hash
            int dist = Hasher::hamming(cur_oper.face_hash, oper.face_hash);
            Log.debug("opers_detect hash dist |", dist);
            return dist < face_hash_thres;
        });
        // 如果两个的hash距离过小，则认为是同一个干员，不进行插入
        if (find_iter != m_all_available_opers.cend()) {
            continue;
        }
        m_all_available_opers.emplace_back(cur_oper);
    }
    return m_all_available_opers.size() - pre_size;
}

bool asst::InfrastProductionTask::optimal_calc()
{
    LogTraceFunction;
    auto& facility_info = InfrastData.get_facility_info(facility_name());
    int cur_max_num_of_opers = facility_info.max_num_of_opers - m_cur_num_of_locked_opers;
    if (m_is_custom) {
        cur_max_num_of_opers -= current_room_config().selected;
    }
    if (cur_max_num_of_opers == 0) {
        Log.warn("no need select opers");
        m_optimal_combs.clear();
        return true;
    }

    std::vector<infrast::SkillsComb> all_available_combs;
    all_available_combs.reserve(m_all_available_opers.size());
    for (auto&& oper : m_all_available_opers) {
        auto comb = efficient_regex_calc(oper.skills);
        comb.name_img = oper.name_img;
        all_available_combs.emplace_back(std::move(comb));
    }

    // 安全获取效率值的辅助函数
    auto get_efficient = [&](const infrast::SkillsComb& comb) -> double {
        auto iter = comb.efficient.find(m_product);
        return iter != comb.efficient.cend() ? iter->second : 0.0;
    };

    // 先把单个的技能按效率排个序，取效率最高的几个
    std::vector<infrast::SkillsComb> optimal_combs;
    optimal_combs.reserve(cur_max_num_of_opers);
    double max_efficient = 0;
    std::ranges::sort(all_available_combs, [&](const infrast::SkillsComb& lhs, const infrast::SkillsComb& rhs) -> bool {
        return get_efficient(lhs) > get_efficient(rhs);
    });

    for (const auto& comb : all_available_combs) {
        std::string skill_str;
        for (const auto& skill : comb.skills) {
            skill_str += skill.id + " ";
        }
        Log.trace(skill_str, get_efficient(comb));
    }

    std::unordered_map<std::string, int> skills_num;
    for (size_t i = 0; i != m_all_available_opers.size(); ++i) {
        const auto& comb = all_available_combs.at(i);

        bool out_of_num = false;
        for (auto&& skill : comb.skills) {
            if (skills_num[skill.id] >= skill.max_num) {
                out_of_num = true;
                break;
            }
        }
        if (out_of_num) {
            continue;
        }

        optimal_combs.emplace_back(comb);
        max_efficient += get_efficient(all_available_combs.at(i));

        for (auto&& skill : comb.skills) {
            ++skills_num[skill.id];
        }

        if (optimal_combs.size() >= static_cast<size_t>(cur_max_num_of_opers)) {
            break;
        }
    }

    {
        std::string log_str = "[ ";
        for (const auto& comb : optimal_combs) {
            log_str += comb.desc.empty() ? comb.skills.begin()->names.front() : comb.desc;
            log_str += "; ";
        }
        log_str += "]";
        Log.trace("Single comb efficient", max_efficient, " , skills:", log_str);
    }

    // 需要选的人和当前房间最大人数不想等，组合就不启用。
    // 可能是房间等级没升满，或者是自定义配置提前选了几个人等
    if (cur_max_num_of_opers != facility_info.max_num_of_opers) {
        m_optimal_combs = std::move(optimal_combs);
        return true;
    }

    // 遍历所有组合，找到效率最高的
    auto& all_group = InfrastData.get_skills_group(facility_name());
    for (const infrast::SkillsGroup& group : all_group) {
        Log.trace(group.desc);
        auto cur_available_opers = all_available_combs;
        bool group_unavailable = false;
        std::vector<infrast::SkillsComb> cur_combs;
        cur_combs.reserve(cur_max_num_of_opers);
        double cur_efficient = 0;
        // 条件判断，不符合的直接过滤掉
        bool meet_condition = true;
        for (const auto& [cond, cond_value] : group.conditions) {
            auto cond_opt = status()->get_number(cond);
            if (!cond_opt) {
                continue;
            }
            // TODO：这里做成除了不等于，还可计算大于、小于等不同条件的
            int cur_value = static_cast<int>(cond_opt.value());
            if (cur_value != cond_value) {
                meet_condition = false;
                break;
            }
        }
        if (!meet_condition) {
            continue;
        }
        // necessary里的技能，一个都不能少
        // TODO necessary暂时没做hash校验。因为没有需要比hash的necessary干员（
        for (const infrast::SkillsComb& nec_skills : group.necessary) {
            auto find_iter = std::ranges::find_if(cur_available_opers, [&](const infrast::SkillsComb& arg) -> bool {
                return arg == nec_skills;
            });
            if (find_iter == cur_available_opers.cend()) {
                group_unavailable = true;
                break;
            }
            cur_combs.emplace_back(nec_skills);
            if (auto iter = nec_skills.efficient_regex.find(m_product); iter != nec_skills.efficient_regex.cend()) {
                auto calc_comb = efficient_regex_calc(nec_skills.skills);
                auto calc_iter = calc_comb.efficient.find(m_product);
                cur_efficient += calc_iter != calc_comb.efficient.cend() ? calc_iter->second : 0.0;
            }
            else {
                auto nec_iter = nec_skills.efficient.find(m_product);
                cur_efficient += nec_iter != nec_skills.efficient.cend() ? nec_iter->second : 0.0;
            }
            cur_available_opers.erase(find_iter);
        }
        if (group_unavailable) {
            continue;
        }
        // 排个序，因为产物不同，效率可能会发生变化，所以配置文件里默认的顺序不一定准确
        auto optional = group.optional;
        for (auto&& opt : optional) {
            if (auto iter = opt.efficient_regex.find(m_product); iter != opt.efficient_regex.cend()) {
                opt = efficient_regex_calc(opt.skills);
            }
        }

        std::ranges::sort(optional, [&](const infrast::SkillsComb& lhs, const infrast::SkillsComb& rhs) -> bool {
            auto lhs_iter = lhs.efficient.find(m_product);
            auto rhs_iter = rhs.efficient.find(m_product);
            double lhs_eff = lhs_iter != lhs.efficient.cend() ? lhs_iter->second : 0.0;
            double rhs_eff = rhs_iter != rhs.efficient.cend() ? rhs_iter->second : 0.0;
            return lhs_eff > rhs_eff;
        });

        // 可能有多个干员有同样的技能，所以这里需要循环找同一个技能，直到找不到为止
        for (const infrast::SkillsComb& opt : optional) {
            auto find_iter = cur_available_opers.cbegin();
            while (cur_combs.size() != static_cast<size_t>(cur_max_num_of_opers)) {
                find_iter =
                    std::find_if(find_iter, cur_available_opers.cend(), [&](const infrast::SkillsComb& arg) -> bool {
                        return arg == opt;
                    });
                if (find_iter != cur_available_opers.cend()) {
                    bool hash_matched = false;
                    if (opt.name_filter.empty()) {
                        hash_matched = true;
                    }
                    else {
                        RegionOCRer name_analyzer(find_iter->name_img);
                        name_analyzer.set_replace(
                            Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map,
                            Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_full);
                        Log.trace("Analyze name filter");
                        if (name_analyzer.analyze()) {
                            std::string name = name_analyzer.get_result().text;
                            hash_matched = std::ranges::find(opt.name_filter, name) != opt.name_filter.cend();
                        }
                        else {
                            hash_matched = false;
                        }
                    }
                    if (!hash_matched) {
                        ++find_iter;
                        continue;
                    }

                    cur_combs.emplace_back(opt);
                    auto opt_iter = opt.efficient.find(m_product);
                    cur_efficient += opt_iter != opt.efficient.cend() ? opt_iter->second : 0.0;
                    find_iter = cur_available_opers.erase(find_iter);
                }
                else {
                    break;
                }
            }
        }

        // 说明可选的没凑满人
        if (cur_combs.size() < static_cast<size_t>(cur_max_num_of_opers)) {
            // 允许外部的话，就把单个干员凑进来
            if (group.allow_external) {
                size_t substitutes = cur_max_num_of_opers - cur_combs.size();
                for (size_t index = 0; index < substitutes; ++index) {
                    const auto& comb = cur_available_opers.at(index);
                    cur_combs.emplace_back(comb);
                    auto comb_iter = comb.efficient.find(m_product);
                    cur_efficient += comb_iter != comb.efficient.cend() ? comb_iter->second : 0.0;
                }
            }
            else { // 否则这个组合人不够，就不可用了
                continue;
            }
        }
        {
            std::string log_str = "[ ";
            for (const auto& comb : cur_combs) {
                log_str += comb.desc.empty() ? comb.skills.begin()->names.front() : comb.desc;
                log_str += "; ";
            }
            log_str += "]";
            Log.trace(group.desc, "efficient", cur_efficient, " , skills:", log_str);
        }

        if (cur_efficient > max_efficient) {
            optimal_combs = std::move(cur_combs);
            max_efficient = cur_efficient;
        }
    }
    {
        std::string log_str = "[ ";
        for (const auto& comb : optimal_combs) {
            log_str += comb.desc.empty() ? comb.skills.begin()->names.front() : comb.desc;
            log_str += "; ";
        }
        log_str += "]";
        Log.trace("optimal efficient", max_efficient, " , skills:", log_str);
    }

    m_optimal_combs = std::move(optimal_combs);

    return true;
}

bool asst::InfrastProductionTask::opers_choose()
{
    LogTraceFunction;
    bool has_error = false;

    auto& facility_info = InfrastData.get_facility_info(facility_name());
    int cur_max_num_of_opers = facility_info.max_num_of_opers - m_cur_num_of_locked_opers;

    const int face_hash_thres = Task.get("InfrastOperFace")->special_params[0];

    int count = 0;
    int swipe_times = 0;

    while (true) {
        if (need_exit()) {
            return false;
        }
        const auto image = ctrler()->get_image();
        InfrastOperImageAnalyzer oper_analyzer(image);
        oper_analyzer.set_to_be_calced(InfrastOperImageAnalyzer::ToBeCalced::All);
        oper_analyzer.set_facility(facility_name());
        if (!oper_analyzer.analyze()) {
            return false;
        }
        oper_analyzer.sort_by_loc();

        // 这个情况一般是滑动/识别出错了，把所有的干员都滑过去了
        if (oper_analyzer.get_num_of_opers_with_skills() == 0) {
            if (!has_error) {
                has_error = true;
                // 倒回去再来一遍
                swipe_to_the_left_of_operlist();
                continue;
            }
            else {
                // 如果已经出过一次错了，那就可能不是opers_choose出错，而是之前的opers_detect出错了
                return false;
            }
        }
        auto cur_all_opers = oper_analyzer.get_result();
        Log.trace("before mood filter, opers size:", cur_all_opers.size());
        // 小于心情阈值的干员则不可用
        std::erase_if(cur_all_opers, [&](const infrast::Oper& rhs) -> bool {
            return rhs.mood_ratio < m_mood_threshold;
        });
        Log.trace("after mood filter, opers size:", cur_all_opers.size());
        for (auto opt_iter = m_optimal_combs.begin(); opt_iter != m_optimal_combs.end();) {
            Log.trace("to find", opt_iter->skills.begin()->names.front());
            auto find_iter = std::ranges::find_if(cur_all_opers, [&](const infrast::Oper& lhs) -> bool {
                if (lhs.skills != opt_iter->skills) {
                    return false;
                }
                if (opt_iter->name_filter.empty()) {
                    return true;
                }
                else {
                    RegionOCRer name_analyzer(lhs.name_img);
                    name_analyzer.set_replace(
                        Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_map,
                        Task.get<OcrTaskInfo>("CharsNameOcrReplace")->replace_full);
                    Log.trace("Analyze name filter");
                    if (!name_analyzer.analyze()) {
                        return false;
                    }
                    std::string name = name_analyzer.get_result().text;
                    return std::ranges::find(std::as_const(opt_iter->name_filter), name) !=
                           opt_iter->name_filter.cend();
                }
            });

            if (find_iter == cur_all_opers.cend()) {
                ++opt_iter;
                Log.trace("not found in this page");
                continue;
            }
            Log.trace("found in this page");
            // 这种情况可能是需要选择两个同样的技能，上一次循环选了一个，但是没有把滑出当前页面，本次又识别到了这个已选择的人
            if (find_iter->selected == true) {
                if (cur_max_num_of_opers != 1) {
                    cur_all_opers.erase(find_iter);
                    Log.trace("skill matched, but it's selected, pass");
                    continue;
                }
                // 但是如果当前设施只有一个位置，即不存在“上次循环”的情况，说明是清除干员按钮没点到
            }
            else {
                ctrler()->click(find_iter->rect);
            }
            {
                auto avlb_iter = std::ranges::find_if(m_all_available_opers, [&](const infrast::Oper& lhs) -> bool {
                    int dist = Hasher::hamming(lhs.face_hash, find_iter->face_hash);
                    Log.debug("opers_choose | face hash dist", dist);
                    return dist < face_hash_thres;
                });
                if (avlb_iter != m_all_available_opers.cend()) {
                    m_all_available_opers.erase(avlb_iter);
                }
                else {
                    Log.error("opers_choose | not found oper");
                }
            }
            ++count;
            cur_all_opers.erase(find_iter);
            opt_iter = m_optimal_combs.erase(opt_iter);
        }
        if (m_optimal_combs.empty()) {
            Log.trace(__FUNCTION__, "| count", count, "cur_max_num_of_opers", cur_max_num_of_opers);
            if (count < cur_max_num_of_opers) {
                // 这种情况可能是萌新，可用干员人数不足以填满当前设施
                callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("NotEnoughStaff"));
            }
            break;
        }

        // 因为识别完了还要点击，所以这里不能异步滑动
        swipe_of_operlist();
        ++swipe_times;
    }

    if (swipe_times) {
        swipe_to_the_left_of_operlist(swipe_times + 1);
    }
    // 点两次排序，让已选干员排到最前面
    ProcessTask(*this, { "InfrastOperListTabSkillUnClicked" }).run();
    ProcessTask(*this, { "InfrastOperListTabWorkStatusUnClicked" }).run();
    return select_opers_review(current_room_config(), count);
}

bool asst::InfrastProductionTask::use_drone()
{
    std::string task_name = "DroneAssist" + facility_name();
    ProcessTask task_temp(*this, { task_name });
    return task_temp.run();
}

asst::infrast::SkillsComb
    asst::InfrastProductionTask::efficient_regex_calc(std::unordered_set<infrast::Skill> skills) const
{
    infrast::SkillsComb comb(std::move(skills));
    // 根据正则，计算当前干员的实际效率
    for (auto&& [product, formula] : comb.efficient_regex) {
        std::string cur_formula = formula;
        for (size_t pos = 0; pos != std::string::npos;) {
            pos = cur_formula.find('[', pos);
            if (pos == std::string::npos) {
                break;
            }
            size_t rp_pos = cur_formula.find(']', pos);
            if (rp_pos == std::string::npos) {
                break;
                // TODO 报错！
            }
            std::string status_key = cur_formula.substr(pos + 1, rp_pos - pos - 1);
            auto status_opt = status()->get_number(status_key);
            const int64_t status_value = status_opt ? status_opt.value() : 0;
            cur_formula.replace(pos, rp_pos - pos + 1, std::to_string(status_value));
        }

        int eff = calculator::eval(cur_formula);
        comb.efficient[product] = eff;
    }
    return comb;
}

bool asst::InfrastProductionTask::facility_list_detect()
{
    LogTraceFunction;
    m_facility_list_tabs.clear();

    const auto image = ctrler()->get_image();
    MultiMatcher mm_analyzer(image);

    mm_analyzer.set_task_info("InfrastFacilityListTab" + facility_name());

    auto result_opt = mm_analyzer.analyze();
    if (!result_opt) {
        return false;
    }
    sort_by_vertical_(*result_opt);
    const auto result = mm_analyzer.get_result();
    for (const auto& res : *result_opt) {
        m_facility_list_tabs.emplace_back(res.rect);
    }

    return true;
}
