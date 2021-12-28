#include "InfrastProductionTask.h"

#include <algorithm>

#include <calculator/calculator.hpp>

#include "AsstUtils.hpp"
#include "Controller.h"
#include "InfrastOperImageAnalyzer.h"
#include "Logger.hpp"
#include "MatchImageAnalyzer.h"
#include "MultiMatchImageAnalyzer.h"
#include "Resource.h"
#include "RuntimeStatus.h"
#include "ProcessTask.h"

bool asst::InfrastProductionTask::shift_facility_list()
{
    LogTraceFunction;
    if (!facility_list_detect()) {
        return false;
    }
    if (need_exit()) {
        return false;
    }
    const auto tab_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        task.get("InfrastFacilityListTab" + m_facility));
    MatchImageAnalyzer add_analyzer;
    const auto add_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        task.get("InfrastAddOperator" + m_facility + m_work_mode_name));
    add_analyzer.set_task_info(*add_task_ptr);

    const auto locked_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        task.get("InfrastOperLocked" + m_facility));
    MultiMatchImageAnalyzer locked_analyzer;
    locked_analyzer.set_task_info(*locked_task_ptr);

    int index = 0;
    for (const Rect& tab : m_facility_list_tabs) {
        if (need_exit()) {
            return false;
        }
        if (index != 0) {
            json::value enter_json = json::object{
                { "facility", m_facility },
                { "index", index }
            };
            m_callback(AsstMsg::EnterFacility, enter_json, m_callback_arg);
        }

        ++index;

        Ctrler.click(tab);
        sleep(tab_task_ptr->rear_delay);

        /* 识别当前制造/贸易站有没有添加干员按钮，没有就不换班 */
        const auto image = Ctrler.get_image();
        add_analyzer.set_image(image);
        if (!add_analyzer.analyze()) {
            Log.info("no add button, just continue");
            continue;
        }
        auto& rect = add_analyzer.get_result().rect;
        Rect add_button = rect;
        auto& rect_move = add_task_ptr->rect_move;
        if (!rect_move.empty()) {
            add_button.x += rect_move.x;
            add_button.y += rect_move.y;
            add_button.width = rect_move.width;
            add_button.height = rect_move.height;
        }

        /* 识别当前正在造什么 */
        MatchImageAnalyzer product_analyzer(image);
        auto& all_products = Resrc.infrast().get_facility_info(m_facility).products;
        std::string cur_product = all_products.at(0);
        double max_score = 0;
        for (const std::string& product : all_products) {
            const static std::string prefix = "InfrastFlag";
            const auto task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
                task.get(prefix + product));
            product_analyzer.set_task_info(*task_ptr);
            if (product_analyzer.analyze()) {
                double score = product_analyzer.get_result().score;
                if (score > max_score) {
                    max_score = score;
                    cur_product = product;
                }
            }
        }
        set_product(cur_product);
        Log.info("cur product", cur_product);

        locked_analyzer.set_image(image);
        if (locked_analyzer.analyze()) {
            m_cur_num_of_lokced_opers = locked_analyzer.get_result().size();
        }
        else {
            m_cur_num_of_lokced_opers = 0;
        }

        /* 进入干员选择页面 */
        Ctrler.click(add_button);
        sleep(add_task_ptr->rear_delay);

        for (int i = 0; i <= OperSelectRetryTimes; ++i) {
            if (need_exit()) {
                return false;
            }
            click_clear_button();

            if (m_all_available_opers.empty()) {
                if (!opers_detect_with_swipe()) {
                    return false;
                }
                swipe_to_the_left_of_operlist(2);
            }
            else {
                opers_detect();
            }
            optimal_calc();
            bool ret = opers_choose();
            if (!ret) {
                m_all_available_opers.clear();
                swipe_to_the_left_of_operlist(2);
                continue;
            }
            break;
        }
        click_confirm_button();

        // 使用无人机
        if (cur_product == m_uses_of_drones) {
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

    int first_number = 0;
    while (true) {
        if (need_exit()) {
            return false;
        }
        size_t num = opers_detect();
        Log.trace("opers_detect return", num);

        // 无论如何也不会没有干员的，除非前面的操作哪里出错了，没进到干员选择的页面。那也就没必要继续下去了
        if (num == 0) {
            return false;
        }

        // 这里本来是判断不相等就可以退出循环。
        // 但是有时候滑动会把一个干员挡住一半，一个页面完整的干员真的只有10个，所以加个2的差值
        if (max_num_of_opers_per_page - num > 2) {
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
    const auto image = Ctrler.get_image();

    InfrastOperImageAnalyzer oper_analyzer(image);
    oper_analyzer.set_facility(m_facility);

    if (!oper_analyzer.analyze()) {
        return 0;
    }
    const auto& cur_all_opers = oper_analyzer.get_result();
    max_num_of_opers_per_page = (std::max)(max_num_of_opers_per_page, cur_all_opers.size());

    int cur_available_num = cur_all_opers.size();
    for (const auto& cur_oper : cur_all_opers) {
        if (cur_oper.skills.empty()) {
            --cur_available_num;
            continue;
        }
        // 心情过低的干员则不可用
        if (cur_oper.mood_ratio < m_mood_threshold) {
            //--cur_available_num;
            continue;
        }
        auto find_iter = std::find_if(
            m_all_available_opers.cbegin(), m_all_available_opers.cend(),
            [&cur_oper](const infrast::Oper& oper) -> bool {
                // 有可能是同一个干员，比一下hash
                int dist = utils::hamming(cur_oper.face_hash, oper.face_hash);
#ifdef LOG_TRACE
                Log.trace("opers_detect hash dist |", dist);
#endif
                return dist < m_face_hash_thres;
            });
        // 如果两个的hash距离过小，则认为是同一个干员，不进行插入
        if (find_iter != m_all_available_opers.cend()) {
            continue;
        }
        m_all_available_opers.emplace_back(cur_oper);
    }
    return cur_available_num;
}

bool asst::InfrastProductionTask::optimal_calc()
{
    LogTraceFunction;
    auto& facility_info = Resrc.infrast().get_facility_info(m_facility);
    int cur_max_num_of_opers = facility_info.max_num_of_opers - m_cur_num_of_lokced_opers;

    std::vector<infrast::SkillsComb> all_avaliable_combs;
    all_avaliable_combs.reserve(m_all_available_opers.size());
    for (auto&& oper : m_all_available_opers) {
        auto comb = efficient_regex_calc(oper.skills);
        comb.name_hash = oper.name_hash;
        all_avaliable_combs.emplace_back(std::move(comb));
    }

    // 先把单个的技能按效率排个序，取效率最高的几个
    std::vector<infrast::SkillsComb> optimal_combs;
    optimal_combs.reserve(cur_max_num_of_opers);
    double max_efficient = 0;
    std::sort(all_avaliable_combs.begin(), all_avaliable_combs.end(),
        [&](const infrast::SkillsComb& lhs, const infrast::SkillsComb& rhs) -> bool {
            return lhs.efficient.at(m_product) > rhs.efficient.at(m_product);
        });

    for (const auto& comb : all_avaliable_combs) {
        std::string skill_str;
        for (const auto& skill : comb.skills) {
            skill_str += skill.id + " ";
        }
        Log.trace(skill_str, comb.efficient.at(m_product));
    }

    std::unordered_map<std::string, int> skills_num;
    for (int i = 0; i != m_all_available_opers.size(); ++i) {
        auto comb = all_avaliable_combs.at(i);

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
        max_efficient += all_avaliable_combs.at(i).efficient.at(m_product);

        for (auto&& skill : comb.skills) {
            ++skills_num[skill.id];
        }

        if (optimal_combs.size() >= cur_max_num_of_opers) {
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

    // 如果有被锁住的干员，说明当前基建没升满级，组合就不启用
    if (m_cur_num_of_lokced_opers != 0) {
        m_optimal_combs = std::move(optimal_combs);
        return true;
    }

    // 遍历所有组合，找到效率最高的
    auto& all_group = Resrc.infrast().get_skills_group(m_facility);
    for (const infrast::SkillsGroup& group : all_group) {
        LogTraceScope(group.desc);
        auto cur_available_opers = all_avaliable_combs;
        bool group_unavailable = false;
        std::vector<infrast::SkillsComb> cur_combs;
        cur_combs.reserve(cur_max_num_of_opers);
        double cur_efficient = 0;
        // 条件判断，不符合的直接过滤掉
        bool meet_condition = true;
        for (const auto& [cond, cond_value] : group.conditions) {
            if (!status.exist(cond)) {
                continue;
            }
            // TODO：这里做成除了不等于，还可计算大于、小于等不同条件的
            int cur_value = status.get(cond);
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
            auto find_iter = std::find_if(
                cur_available_opers.cbegin(), cur_available_opers.cend(),
                [&](const infrast::SkillsComb& arg) -> bool {
                    return arg == nec_skills;
                });
            if (find_iter == cur_available_opers.cend()) {
                group_unavailable = true;
                break;
            }
            cur_combs.emplace_back(nec_skills);
            if (auto iter = nec_skills.efficient_regex.find(m_product);
                iter != nec_skills.efficient_regex.cend()) {
                cur_efficient += efficient_regex_calc(nec_skills.skills).efficient.at(m_product);
            }
            else {
                cur_efficient += nec_skills.efficient.at(m_product);
            }
            cur_available_opers.erase(find_iter);
        }
        if (group_unavailable) {
            continue;
        }
        // 排个序，因为产物不同，效率可能会发生变化，所以配置文件里默认的顺序不一定准确
        auto optional = group.optional;
        for (auto&& opt : optional) {
            if (auto iter = opt.efficient_regex.find(m_product);
                iter != opt.efficient_regex.cend()) {
                opt = efficient_regex_calc(opt.skills);
            }
        }

        std::sort(optional.begin(), optional.end(),
            [&](const infrast::SkillsComb& lhs,
                const infrast::SkillsComb& rhs) -> bool {
                    return lhs.efficient.at(m_product) > rhs.efficient.at(m_product);
            });

        // 可能有多个干员有同样的技能，所以这里需要循环找同一个技能，直到找不到为止
        for (const infrast::SkillsComb& opt : optional) {
            auto find_iter = cur_available_opers.cbegin();
            while (cur_combs.size() != cur_max_num_of_opers) {
                find_iter = std::find_if(
                    find_iter, cur_available_opers.cend(),
                    [&](const infrast::SkillsComb& arg) -> bool {
                        return arg == opt;
                    });
                if (find_iter != cur_available_opers.cend()) {
                    // 要求技能匹配的同时，hash也要匹配
                    bool hash_matched = false;
                    if (!opt.possible_hashs.empty()) {
                        for (const auto& [key, hash] : opt.possible_hashs) {
                            int dist = utils::hamming(find_iter->name_hash, hash);
                            Log.trace("optimal_calc | name hash dist", dist, hash, find_iter->name_hash);
                            if (dist < m_name_hash_thres) {
                                hash_matched = true;
                                break;
                            }
                        }
                    }
                    else {
                        hash_matched = true;
                    }
                    if (!hash_matched) {
                        ++find_iter;
                        continue;
                    }

                    cur_combs.emplace_back(opt);
                    cur_efficient += opt.efficient.at(m_product);
                    find_iter = cur_available_opers.erase(find_iter);
                }
                else {
                    break;
                }
            }
        }

        // 说明可选的没凑满人
        if (cur_combs.size() < cur_max_num_of_opers) {
            // 允许外部的话，就把单个干员凑进来
            if (group.allow_external) {
                for (size_t i = cur_combs.size(); i != cur_max_num_of_opers; ++i) {
                    cur_combs.emplace_back(cur_available_opers.at(i));
                    cur_efficient += cur_available_opers.at(i).efficient.at(m_product);
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

    int count = 0;
    auto& facility_info = Resrc.infrast().get_facility_info(m_facility);
    int cur_max_num_of_opers = facility_info.max_num_of_opers - m_cur_num_of_lokced_opers;

    while (true) {
        if (need_exit()) {
            return false;
        }
        const auto image = Ctrler.get_image();

        InfrastOperImageAnalyzer oper_analyzer(image);
        oper_analyzer.set_facility(m_facility);

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
        // 小于心情阈值的干员则不可用
        auto remove_iter = std::remove_if(cur_all_opers.begin(), cur_all_opers.end(),
            [&](const infrast::Oper& rhs) -> bool {
                return rhs.mood_ratio < m_mood_threshold;
            });
        cur_all_opers.erase(remove_iter, cur_all_opers.end());

        for (auto opt_iter = m_optimal_combs.begin(); opt_iter != m_optimal_combs.end();) {
            auto find_iter = std::find_if(
                cur_all_opers.cbegin(), cur_all_opers.cend(),
                [&](const infrast::Oper& lhs) -> bool {
                    if (lhs.skills != opt_iter->skills) {
                        return false;
                    }
                    if (!opt_iter->hash_filter) {
                        return true;
                    }
                    else {
                        // 既要技能相同，也要hash相同，双重校验
                        for (const auto& [_, hash] : opt_iter->possible_hashs) {
                            int dist = utils::hamming(lhs.name_hash, hash);
                            Log.trace("opers_choose | name hash dist", dist);
                            if (dist < m_name_hash_thres) {
                                return true;
                            }
                        }
                        return false;
                    }
                });

            if (find_iter == cur_all_opers.cend()) {
                ++opt_iter;
                continue;
            }
            // 这种情况可能是需要选择两个同样的技能，上一次循环选了一个，但是没有把滑出当前页面，本次又识别到了这个已选择的人
            if (find_iter->selected == true) {
                if (cur_max_num_of_opers != 1) {
                    cur_all_opers.erase(find_iter);
                    continue;
                }
                // 但是如果当前设施只有一个位置，即不存在“上次循环”的情况，说明是清除干员按钮没点到
            }
            else {
                Ctrler.click(find_iter->rect);
            }
            {
                auto avlb_iter = std::find_if(
                    m_all_available_opers.cbegin(), m_all_available_opers.cend(),
                    [&](const infrast::Oper& lhs) -> bool {
                        int dist = utils::hamming(lhs.face_hash, find_iter->face_hash);
                        Log.trace("opers_choose | face hash dist", dist);
                        if (dist < m_face_hash_thres) {
                            return true;
                        }
                        return false;
                    }
                );
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
            if (count >= cur_max_num_of_opers) {
                break;
            }
            else { // 这种情况可能是萌新，可用干员人数不足以填满当前设施
                // TODO!!!
                break;
            }
        }

        // 因为识别完了还要点击，所以这里不能异步滑动
        swipe_of_operlist();
    }

    return true;
}

bool asst::InfrastProductionTask::use_drone()
{
    std::string task_name = "DroneAssist" + m_facility;
    ProcessTask task(*this, { task_name });
    return task.run();
}

asst::infrast::SkillsComb
asst::InfrastProductionTask::efficient_regex_calc(
    std::unordered_set<infrast::Skill> skills) const
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
            int status_value = status.get(status_key);
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

    const auto image = Ctrler.get_image();
    MultiMatchImageAnalyzer mm_analyzer(image);

    const auto task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
        task.get("InfrastFacilityListTab" + m_facility));
    mm_analyzer.set_task_info(*task_ptr);

    if (!mm_analyzer.analyze()) {
        return false;
    }
    mm_analyzer.sort_result();
    m_facility_list_tabs.reserve(mm_analyzer.get_result().size());
    for (const auto& res : mm_analyzer.get_result()) {
        m_facility_list_tabs.emplace_back(res.rect);
    }

    return true;
}
