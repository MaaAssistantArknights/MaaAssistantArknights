#include "InfrastShiftTask.h"

#include <algorithm>

#include "Resource.h"
#include "Controller.h"
#include "InfrastSkillsImageAnalyzer.h"
#include "AsstUtils.hpp"
#include "Logger.hpp"

bool asst::InfrastShiftTask::run()
{
    json::value task_start_json = json::object{
        { "task_type",  "InfrastShiftTask" },
        { "task_chain", m_task_chain}
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    m_all_available_opers.clear();

    bool ret = opers_detect();

    optimal_calc();

    return false;
}

bool asst::InfrastShiftTask::opers_detect()
{
    int first_number = 0;
    while (true) {
        const auto& image = ctrler.get_image();

        InfrastSkillsImageAnalyzer skills_analyzer(image);
        skills_analyzer.set_facility(m_facility);

        if (!skills_analyzer.analyze()) {
            return false;
        }
        const auto& cur_all_info = skills_analyzer.get_result();
        if (first_number == 0) {
            first_number = cur_all_info.size();
        }
        constexpr static int HashDistThres = 25;
        // 如果两个的hash距离过小，则认为是同一个干员，不进行插入
        for (const auto& cur : cur_all_info) {
            auto find_iter = std::find_if(m_all_available_opers.cbegin(), m_all_available_opers.cend(),
                [&cur](const InfrastOperSkillInfo& info) -> bool {
                    int dist = utils::hamming(cur.hash, info.hash);
                    return dist < HashDistThres;
                });
            if (find_iter == m_all_available_opers.cend()) {
                m_all_available_opers.emplace_back(cur);
            }
        }
        // for debug
        //break;

        // 这里本来是判断不相等就可以退出循环。
        // 但是有时候滑动会把一个干员挡住一半，一个页面完整的干员真的只有10个，所以加个2的差值
        if (first_number - cur_all_info.size() > 2) {
            break;
        }
    }

    if (!m_all_available_opers.empty()) {
        return true;
    }
    return false;
}

bool asst::InfrastShiftTask::optimal_calc()
{
    auto& facility_info = resource.infrast().get_facility_info(m_facility);
    int max_num_of_opers = facility_info.max_num_of_opers;

    if (m_all_available_opers.size() < max_num_of_opers) {
        return false;
    }

    // TODO: 处理效率的正则，将正则的结果计算到数字的efficient中

    // 先把单个的技能按效率排个序，取效率最高的几个
    std::vector<InfrastOperSkillInfo> optimal_opers;
    optimal_opers.reserve(max_num_of_opers);
    int max_efficient = 0;
    std::sort(m_all_available_opers.begin(), m_all_available_opers.end(),
        [&](const InfrastOperSkillInfo& lhs, const InfrastOperSkillInfo& rhs) -> bool {
            return lhs.skills.efficient.at(m_product) > rhs.skills.efficient.at(m_product);
        });
    for (int i = 0; i != max_num_of_opers; ++i) {
        optimal_opers.emplace_back(m_all_available_opers.at(i));
        max_efficient += m_all_available_opers.at(i).skills.efficient.at(m_product);
    }

#ifdef LOG_TRACE
    for (const auto& oper : m_all_available_opers) {
        log.trace(oper.skills.skills.begin()->id);
    }
#endif // LOG_TRACE

    // 遍历所有组合，找到效率最高的
    auto& all_group = resource.infrast().get_skills_group(m_facility);
    for (const InfrastSkillsGroup& group : all_group) {
        auto cur_available_opers = m_all_available_opers;
        bool group_unavailable = false;
        std::vector<InfrastOperSkillInfo> cur_opers;
        cur_opers.reserve(max_num_of_opers);
        int cur_efficient = 0;
        // TODO：条件判断，不符合的直接过滤掉
        for (const auto& [cond, value] : group.conditions) {
            // if xxx continue;
        }
        // necessary里的技能，一个都不能少
        for (const InfrastSkillsComb& nec_skills : group.necessary) {
            auto find_iter = std::find_if(cur_available_opers.cbegin(), cur_available_opers.cend(),
                [&](const InfrastOperSkillInfo& arg) -> bool {
                    return arg.skills == nec_skills;
                });
            if (find_iter == cur_available_opers.cend()) {
                group_unavailable = true;
                break;
            }
            cur_opers.emplace_back(nec_skills);
            cur_efficient += nec_skills.efficient.at(m_product);
            cur_available_opers.erase(find_iter);
        }
        if (group_unavailable) {
            continue;
        }
        // 排个序，因为产物不同，效率可能会发生变化，所以配置文件里默认的顺序不一定准确
        auto optional = group.optional;
        std::sort(optional.begin(), optional.end(),
            [&](const InfrastSkillsComb& lhs, const InfrastSkillsComb& rhs) -> bool {
                return lhs.efficient.at(m_product) > rhs.efficient.at(m_product);
            });

        // 可能有多个干员有同样的技能，所以这里需要循环找同一个技能，直到找不到为止
        for (const InfrastSkillsComb& opt : optional) {
            auto find_iter = cur_available_opers.cbegin();
            while (cur_opers.size() != max_num_of_opers) {
                find_iter = std::find_if(find_iter, cur_available_opers.cend(),
                    [&](const InfrastOperSkillInfo& arg) -> bool {
                        return arg.skills.skills == opt.skills;
                    });
                if (find_iter != cur_available_opers.cend()) {
                    cur_opers.emplace_back(opt);
                    cur_efficient += opt.efficient.at(m_product);
                    find_iter = cur_available_opers.erase(find_iter);
                }
                else {
                    break;
                }
            }
        }

        // 说明可选的没凑满人
        if (cur_opers.size() < max_num_of_opers) {
            // 允许外部的话，就把单个干员凑进来
            if (group.allow_external) {
                for (size_t i = cur_opers.size(); i != max_num_of_opers; ++i) {
                    cur_opers.emplace_back(cur_available_opers.at(i));
                    cur_efficient += cur_available_opers.at(i).skills.efficient.at(m_product);
                }
            }
            else { // 否则这个组合人不够，就不可用了
                continue;
            }
        }
        if (cur_efficient > max_efficient) {
            optimal_opers = std::move(cur_opers);
            max_efficient = cur_efficient;
        }
    }

    std::string log_str = "[ ";
    for (const auto& oper : optimal_opers) {
        log_str += oper.skills.intro.empty() ? oper.skills.skills.begin()->names.front() : oper.skills.intro;
        log_str += "; ";
    }
    log_str += "]";
    log.trace("optimal efficient", max_efficient, " , skills:", log_str);


    m_optimal_opers = std::move(optimal_opers);

    return true;
}