#include "FightTimesTaskPlugin.h"

#include "Controller/Controller.h"
#include "Task/ProcessTask.h"

bool asst::FightTimesTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    return !inited && details.get("details", "task", "").ends_with("StartButton1");
}

bool asst::FightTimesTaskPlugin::_run()
{
    const static std::string FightSeriesOpenTask = "FightSeries-Indicator";
    const static std::string FightSeriesValidTask = "FightSeries-Icon";

    auto img = ctrler()->get_image();
    auto is_valid_task = ProcessTask(*this, { FightSeriesOpenTask, FightSeriesValidTask });
    is_valid_task.set_reusable_image(img).set_retry_times(0);

    if (!is_valid_task.run()) { // 认为是外服，无法使用连续战斗
        inited = true;
        return true;
    }

    if (is_valid_task.get_last_task_name() == FightSeriesValidTask) { // 连续战斗次数选择列表未打开
        auto task = ProcessTask(*this, { "FightSeries-Open" });
        task.set_reusable_image(img);
        if (!task.run()) {
            return false;
        }
    }
    ProcessTask(*this, { "FightSeries-List-" + std::to_string(m_series) }).run();

    inited = true;
    return true;
}
