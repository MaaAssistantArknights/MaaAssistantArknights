#include "RoguelikeRecruitTaskPlugin.h"

#include "OcrImageAnalyzer.h"
#include "Controller.h"
#include "TaskData.h"
#include "Resource.h"
#include "Logger.hpp"

void asst::RoguelikeRecruitTaskPlugin::set_opers(std::vector<std::string> opers)
{
    m_opers = std::move(opers);
}

bool asst::RoguelikeRecruitTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted
        || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (details.at("details").at("task").as_string() == "Roguelike1ChooseOper") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeRecruitTaskPlugin::_run()
{
    LogTraceFunction;

    OcrImageAnalyzer analyzer(m_ctrler->get_image());
    analyzer.set_task_info("Roguelike1RecruitData");

    // 这里原来用m_opers作参数，但由于新的算法需要取左上角干员，将m_opers移到下面的demand_filter
    // TODO 这里需要考虑出了新干员，但是资源文件没有更新的情况
    analyzer.set_required(Resrc.combatrecruit().get_all_opers_name());

    if (!analyzer.analyze()) {
        return false;
    }

    analyzer.sort_result_x_y();

    {
        std::string visible_opers = ""; // 界面上所有干员
        for (const auto& oper : analyzer.get_result()) {
            visible_opers += oper.text + ", ";
        }
        Log.info("visible opers count：", analyzer.get_result().size(), "visible opers：", visible_opers);
    }
    // demand_filter
    std::unordered_set<std::string> opers_set(m_opers.cbegin(), m_opers.cend());
    analyzer.filter([&](const TextRect& x) -> bool {
        return opers_set.find(x.text) != opers_set.cend();
        }
    );

    {
        std::string required_opers = ""; // visible_opers 的基础上, 在 m_opers 里的干员
        for (const auto& oper : analyzer.get_result()) {
            required_opers += oper.text + ", ";
        }
        Log.info("required opers count：", analyzer.get_result().size(), "required opers：", required_opers);
    }

    if (analyzer.get_result().empty()) {
        return false;
    }
    // 1. 根据出现在界面左上角的干员确定最高星级（此前应已判断第一个是否为临时招募）
    const auto& all_opers = Resrc.combatrecruit().get_all_opers();
    const std::string& first_oper = analyzer.get_result().front().text;

    if (auto oper_iter = all_opers.find(first_oper);
        oper_iter != all_opers.end()) {
        int allow_level = oper_iter->second.level;
        Log.info("allow level：", allow_level);
        // availability_filter (based on level)
        analyzer.filter([&](const TextRect& tr) -> bool {
            if (auto iter = all_opers.find(tr.text);
                iter != all_opers.end()) {
                return iter->second.level <= allow_level;
            }
            else {
                return false;
            }
        });
    }
    else {
        Log.error("Cannot recognize first oper：", first_oper);
    }

    {
        std::string available_opers = ""; // required_opers 的基础上, 能用的干员
        for (const auto& oper : analyzer.get_result()) {
            available_opers += oper.text + ", ";
        }
        Log.info("available opers count：", analyzer.get_result().size(), "available opers：", available_opers);
    }

    if (analyzer.get_result().empty()) {
        return false;
    }
    // 2. 根据指定的干员的先后顺序决定人选（越靠前优先级越高）
    analyzer.set_required(m_opers);
    analyzer.sort_result_by_required();
    Rect target = analyzer.get_result().front().rect;
    m_ctrler->click(target);
    Log.info("Choose：", analyzer.get_result().front().text);

    return true;
}
