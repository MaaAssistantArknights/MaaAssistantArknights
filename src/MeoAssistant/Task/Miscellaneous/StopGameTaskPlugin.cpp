#include "StopGameTaskPlugin.h"

#include "Controller.h"

using namespace asst;

bool StopGameTaskPlugin::_run()
{
    return m_ctrler->stop_game();
}
