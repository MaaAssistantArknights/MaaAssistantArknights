#include "StopGameTaskPlugin.h"

#include "Controller.h"

using namespace asst;

bool StopGameTaskPlugin::_run()
{
    return ctrler()->stop_game();
}
