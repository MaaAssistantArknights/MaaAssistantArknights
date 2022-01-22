#include "RoguelikeFormationTask.h"

#include "RoguelikeFormationImageAnalyzer.h"
#include "Controller.h"
#include "TaskData.h"
#include "ProcessTask.h"

bool asst::RoguelikeFormationTask::_run()
{
    RoguelikeFormationImageAnalyzer formation_analyzer(Ctrler.get_image());
    if (!formation_analyzer.analyze()) {
        return false;
    }

    for (const auto& oper : formation_analyzer.get_result()) {
        if (oper.selected) {
            continue;
        }
        Ctrler.click(oper.rect);
    }

    return click_confirm();
}

bool asst::RoguelikeFormationTask::click_confirm()
{
    ProcessTask task(*this, { "Roguelike1FormationConfirm" });
    return task.run();
}
