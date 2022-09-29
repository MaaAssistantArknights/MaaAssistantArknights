#include "DebugTask.h"

#include <filesystem>

#include "Utils/NoWarningCV.h"

// #include "Plugin/RoguelikeSkillSelectionTaskPlugin.h"

#include "ImageAnalyzer/DepotImageAnalyzer.h"
#include "ImageAnalyzer/StageDropsImageAnalyzer.h"
#include "Utils/AsstImageIo.hpp"
#include "Utils/Logger.hpp"

asst::DebugTask::DebugTask(const AsstCallback& callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType)
{
    ////auto task_ptr = std::make_shared<RoguelikeSkillSelectionTaskPlugin>(callback, callback_arg, TaskType);
    // auto task_ptr = std::make_shared<StageDropsTaskPlugin>(callback, callback_arg, TaskType);
    // m_subtasks.emplace_back(task_ptr);
}

bool asst::DebugTask::run()
{
    size_t total = 0;
    size_t success = 0;
    for (const auto& entry : std::filesystem::directory_iterator("../../test/depot/screenshots")) {
        cv::Mat image = asst::imread(entry.path());
        if (image.empty()) {
            continue;
        }
        total += 1;
        cv::Mat resized;
        cv::resize(image, resized, cv::Size(1280, 720), 0, 0, cv::INTER_AREA);
        DepotImageAnalyzer analyzer(resized);
        success += analyzer.analyze();
    }
    Log.info(__FUNCTION__, success, "/", total);
    return true;
}
