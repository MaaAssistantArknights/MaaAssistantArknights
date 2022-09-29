#include "InfrastInfoTask.h"

#include "Controller.h"
#include "ImageAnalyzer/InfrastFacilityImageAnalyzer.h"
#include "RuntimeStatus.h"
#include "Utils/Logger.hpp"

bool asst::InfrastInfoTask::_run()
{
    swipe_to_the_left_of_main_ui();
    const auto image = m_ctrler->get_image();

    InfrastFacilityImageAnalyzer analyzer(image);

    analyzer.set_to_be_analyzed({ "Mfg", "Trade", "Power", "Dorm" });
    if (!analyzer.analyze()) {
        return false;
    }
    for (auto&& [name, res] : analyzer.get_result()) {
        std::string key = "NumOf" + name;
        // int size = static_cast<int>(res.size());
        m_status->set_number(key, res.size());
        Log.trace("InfrastInfoTask | ", key, res.size());
    }

    return true;
}
