#include "SSSDropRewardsTaskPlugin.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/SSSCopilotConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Task/SSS/SSSBattleProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

bool asst::SSSDropRewardsTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string& task = details.get("details", "task", "");
    return task == "SSSDropRecruitmentFlag";
}

bool asst::SSSDropRewardsTaskPlugin::_run()
{
    using namespace battle;
    LogTraceFunction;

    OCRer analyzer(ctrler()->get_image());
    analyzer.set_task_info("SSSDropRecruitmentOCR");
    if (!analyzer.analyze()) {
        Log.error(__FUNCTION__, "OCR failed to analyze");
        return false;
    }

    struct DropRecruitment
    {
        TextRect ocr_res;
        std::optional<Role> role;
    };

    std::vector<DropRecruitment> opers;
    for (const auto& result : analyzer.get_result()) {
        if (SSSCopilot.get_data().blacklist.contains(result.text)) {
            Log.info("Operator is blacklisted:", result.text);
            continue;
        }
        auto role = BattleData.get_role(result.text);
        opers.emplace_back(
            DropRecruitment { .ocr_res = result,
                              .role = role == Role::Unknown ? std::nullopt : std::optional<Role>(role) });
    }

    bool operSelect = false;
    for (const std::string& name : SSSCopilot.get_data().order_of_drops) {
        auto iter = ranges::find_if(opers, [&](const auto& props) {
            auto role = get_role_type(name);
            return (role != Role::Unknown && props.role) ? *props.role == role : props.ocr_res.text == name;
        });
        if (iter != opers.cend()) {
            ctrler()->click(iter->ocr_res.rect);
            sleep(Task.get("SSSDropRecruitmentOCR")->post_delay);
            operSelect = true;
            break;
        }
    }
    if (!operSelect) {
        Log.warn("No operator selected. Bypassing blacklist...");
        ctrler()->click(opers.at(0).ocr_res.rect);
    }

    return ProcessTask(*this, { "SSSDropRecruitmentConfirm" }).set_retry_times(5).run();
}
