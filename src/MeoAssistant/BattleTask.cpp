#include "BattleTask.h"

#include "BattleImageAnalyzer.h"
#include "Controller.h"

bool asst::BattleTask::_run()
{
    BattleImageAnalyzer oper_analyzer(Ctrler.get_image());
    oper_analyzer.analyze();
    return false;
}
