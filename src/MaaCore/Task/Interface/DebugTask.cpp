#include "DebugTask.h"

#include <filesystem>

#include "Utils/NoWarningCV.h"

// #include "Plugin/RoguelikeSkillSelectionTaskPlugin.h"

#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Miscellaneous/DepotImageAnalyzer.h"
#include "Vision/Miscellaneous/StageDropsImageAnalyzer.h"

asst::DebugTask::DebugTask(const AsstCallback& callback, Assistant* inst) : InterfaceTask(callback, inst, TaskType)
{
    ////auto task_ptr = std::make_shared<RoguelikeSkillSelectionTaskPlugin>(callback, inst, TaskType);
    // auto task_ptr = std::make_shared<StageDropsTaskPlugin>(callback, inst, TaskType);
    // m_subtasks.emplace_back(task_ptr);
}

bool asst::DebugTask::run()
{
    size_t total = 0;
    size_t success = 0;
    for (const auto& entry : std::filesystem::directory_iterator("../../test/drops/screenshots/zh_cn")) {
        cv::Mat image = asst::imread(entry.path());
        if (image.empty()) {
            continue;
        }
        total += 1;
        cv::Mat resized;
        cv::resize(image, resized, cv::Size(1280, 720), 0, 0, cv::INTER_AREA);
        StageDropsImageAnalyzer analyzer(resized);
        success += analyzer.analyze();
    }
    Log.info(__FUNCTION__, success, "/", total);
    return true;
}
