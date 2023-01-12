#include "SSSDropRewardsTaskPlugin.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/SSSCopilotConfig.h"
#include "Controller.h"
#include "Task/ProcessTask.h"
#include "Task/SSS/SSSBattleProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OcrImageAnalyzer.h"
#include "Vision/OcrWithPreprocessImageAnalyzer.h"

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

    OcrImageAnalyzer analyzer(ctrler()->get_image());
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
        auto role = BattleData.get_role(result.text);
        opers.emplace_back(DropRecruitment {
            .ocr_res = result, .role = role == Role::Unknown ? std::nullopt : std::optional<Role>(role) });
    }

    for (const std::string& name : SSSCopilot.get_data().drop_tool_men) {
        auto role = get_role_type(name);
        bool is_role = role != Role::Unknown;
        auto iter = ranges::find_if(opers, [&](const DropRecruitment& props) {
            return (is_role && props.role) ? *props.role == role : props.ocr_res.text == name;
        });
        if (iter != opers.cend()) {
            ctrler()->click(iter->ocr_res.rect);
            break;
        }
    }

    return ProcessTask(*this, { "SSSDropRecruitmentConfrim" }).run();
}
