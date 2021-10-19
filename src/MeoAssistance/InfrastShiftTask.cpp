#include "InfrastShiftTask.h"

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
    // TODO: 处理效率的正则

    // 先把单个的技能按效率排个序
    std::sort(m_all_available_opers.begin(), m_all_available_opers.end(),
        [&](const InfrastOperSkillInfo& lhs, const InfrastOperSkillInfo& rhs) -> bool {
            return lhs.skills.efficient.at(m_product) > rhs.skills.efficient.at(m_product);
        });

    return false;
}