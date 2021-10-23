#include "InfrastEnterFacilityTask.h"

#include "InfrastFacilityImageAnalyzer.h"
#include "Resource.h"
#include "Controller.h"

bool asst::InfrastEnterFacilityTask::run()
{
    const auto& image = ctrler.get_image();

    InfrastFacilityImageAnalyzer analyzer(image);
    analyzer.analyze();

    return false;
}