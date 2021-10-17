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

    std::vector<InfrastOperSkillInfo> all_info;
    while (true) {
        const auto& image = ctrler.get_image();

        InfrastSkillsImageAnalyzer skills_analyzer(image);

        if (!skills_analyzer.analyze()) {
            return false;
        }
        const auto& cur_all_info = skills_analyzer.get_result();
        constexpr static int HashDistThres = 25;
        // 如果两个的hash距离过小，则认为是同一个干员，不进行插入
        for (const auto& cur : cur_all_info) {
            auto find_iter = std::find_if(all_info.cbegin(), all_info.cend(),
                [&cur](const InfrastOperSkillInfo& info) -> bool {
                    int dist = utils::hamming(cur.hash, info.hash);
                    return dist < HashDistThres;
                });
            if (find_iter == all_info.cend()) {
                all_info.emplace_back(cur);
            }
        }
    }
    if (!all_info.empty()) {
        return true;
    }

    return false;
}