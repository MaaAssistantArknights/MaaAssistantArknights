#include "DebugTask.h"

#include <filesystem>

#include "Utils/NoWarningCV.h"

#include "Config/TaskData.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Battle/BattlefieldClassifier.h"
#include "Vision/Battle/BattlefieldMatcher.h"
#include "Vision/Matcher.h"
#include "Vision/Miscellaneous/DepotImageAnalyzer.h"
#include "Vision/Miscellaneous/StageDropsImageAnalyzer.h"

asst::DebugTask::DebugTask(const AsstCallback& callback, Assistant* inst) : InterfaceTask(callback, inst, TaskType) {}

bool asst::DebugTask::run()
{
    test_match_template();
    return true;
}

void asst::DebugTask::test_drops()
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
}

void asst::DebugTask::test_skill_ready()
{
    int total = 0;
    int correct = 0;
    for (const auto& entry : std::filesystem::directory_iterator(R"(../../test/skill_ready/y)")) {
        cv::Mat image = imread(entry.path().string());
        BattlefieldClassifier analyzer(image);
        analyzer.set_object_of_interest({ .skill_ready = true });
        total++;
        if (analyzer.analyze()->skill_ready.ready) {
            correct++;
        }
    }
    for (const auto& entry : std::filesystem::directory_iterator(R"(../../test/skill_ready/n)")) {
        cv::Mat image = imread(entry.path().string());
        BattlefieldClassifier analyzer(image);
        analyzer.set_object_of_interest({ .skill_ready = true });
        total++;
        if (!analyzer.analyze()->skill_ready.ready) {
            correct++;
        }
    }
    Log.info(__FUNCTION__, correct, "/", total, ",", double(correct) / total);
}

void asst::DebugTask::test_battle_image()
{
    cv::Mat image = asst::imread(utils::path("1.png"));
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(1280, 720), 0, 0, cv::INTER_AREA);
    BattlefieldMatcher analyzer(resized);
    analyzer.set_object_of_interest({ .deployment = true });
    analyzer.analyze();
}

void asst::DebugTask::test_match_template()
{
    auto test_task = [](const std::string& path, const std::string& task_name) {
        cv::Mat image = imread(utils::path(path));
        cv::Mat resized;
        cv::resize(image, resized, cv::Size(1280, 720), 0, 0, cv::INTER_AREA);
        Matcher match_analyzer(resized, Rect(0, 0, 1280, 720));
        const auto& task_ptr = Task.get(task_name);
        const auto match_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(task_ptr);
        match_analyzer.set_task_info(match_task_ptr);
        const auto& result_opt = match_analyzer.analyze();
        if (result_opt) {
            const auto& result = result_opt.value().to_string();
            Log.info("active", path, task_name, result);
        }
        else {
            Log.info("inactive", path, task_name);
        }
    };

    test_task("../../resource/template/Sarkaz@Roguelike@StageConfrontation.png", "Sarkaz@Roguelike@StageEmergencyTransportation");
}
