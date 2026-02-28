#include "DebugTask.h"

#include "MaaUtils/Encoding.h"
#include <boost/regex.hpp>
#include <filesystem>
#include <shared_mutex>

#include "Common/AsstTypes.h"
#include "Config/TaskData.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Battle/BattlefieldClassifier.h"
#include "Vision/Battle/BattlefieldMatcher.h"
#include "Vision/BestMatcher.h"
#include "Vision/FeatureMatcher.h"
#include "Vision/Matcher.h"
#include "Config/TemplResource.h"
#include "Vision/Miscellaneous/DepotImageAnalyzer.h"
#include "Vision/Miscellaneous/StageDropsImageAnalyzer.h"
#include "Vision/MultiMatcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

asst::DebugTask::DebugTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType)
{
}

bool asst::DebugTask::run()
{
    auto image_dep = MaaNS::imread(utils::path("C:\\Users\\status102\\Desktop\\deploy.png"));
    static const std::unordered_map<std::string, battle::Role> RoleMap = {
        { "Caster", battle::Role::Caster }, { "Medic", battle::Role::Medic },     { "Pioneer", battle::Role::Pioneer },
        { "Sniper", battle::Role::Sniper }, { "Special", battle::Role::Special }, { "Support", battle::Role::Support },
        { "Tank", battle::Role::Tank },     { "Warrior", battle::Role::Warrior }, { "Drone", battle::Role::Drone },
    };

    auto image = MaaNS::imread(utils::path("C:\\Users\\status102\\Desktop\\kill_1080p_Vec.png"));

    cv::Mat template_image = TemplResource::get_instance().get_templ("BattleKillsFlag.png");
    auto task = Task.get("BattleKillsFlag");

    // 对模板进行高斯模糊预处理（解决小尺寸模板的细线条缩放问题）
    cv::Mat template_blurred;
    cv::GaussianBlur(template_image, template_blurred, cv::Size(3, 3), 0);

    // 同时对输入图像应用相同的模糊处理
    cv::Mat image_blurred;
    cv::GaussianBlur(make_roi(image, task->roi), image_blurred, cv::Size(3, 3), 0);

    // 使用模糊后的图像进行匹配
    Matcher analyzer(image_blurred);
    analyzer.set_task_info("BattleKillsFlag");
    analyzer.set_roi({ 0, 0, task->roi.width, task->roi.height });
    if (!analyzer.analyze()) {
        Log.info("flag not found");
    }
    // 使用模糊后的图像进行匹配
    Matcher analyzer2(image_blurred);
    analyzer2.set_task_info("BattleKillsFlag");
    analyzer2.set_templ(template_blurred);
    analyzer2.set_roi({ 0, 0, task->roi.width, task->roi.height });
    if (!analyzer2.analyze()) {
        Log.info("flag not found");
    }

    return true;
}

void asst::DebugTask::test_drops()
{
    size_t total = 0;
    size_t success = 0;
    for (const auto& entry : std::filesystem::directory_iterator("../../test/drops/screenshots/zh_cn")) {
        cv::Mat image = MAA_NS::imread(entry.path());
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

    // 测试 y 类别（预期为 ready，即 true）
    for (const auto& entry : std::filesystem::directory_iterator(R"(../../test/skill_ready/y)")) {
        cv::Mat image = MAA_NS::imread(entry.path());
        BattlefieldClassifier analyzer(image);
        analyzer.set_object_of_interest({ .skill_ready = true });
        total++;
        auto result = analyzer.analyze()->skill_ready;
        // 记录日志：文件、预期结果、实际预测、得分、概率信息
        Log.info(
            __FUNCTION__,
            "File: ",
            entry.path().string(),
            " | Expected: Y (ready: true)",
            " | Predicted: ",
            result.ready,
            " | Score: ",
            result.score,
            " | Prob: ",
            result.prob);
        if (result.ready) {
            correct++;
        }
    }

    // 测试 n 类别（预期为 not ready，即 false）
    for (const auto& entry : std::filesystem::directory_iterator(R"(../../test/skill_ready/n)")) {
        cv::Mat image = MAA_NS::imread(entry.path());
        BattlefieldClassifier analyzer(image);
        analyzer.set_object_of_interest({ .skill_ready = true });
        total++;
        auto result = analyzer.analyze()->skill_ready;
        Log.info(
            __FUNCTION__,
            "File: ",
            entry.path().string(),
            " | Expected: N (ready: false)",
            " | Predicted: ",
            result.ready,
            " | Score: ",
            result.score,
            " | Prob: ",
            result.prob);
        if (!result.ready) {
            correct++;
        }
    }

    // 测试 c 类别（同样预期为 not ready）
    for (const auto& entry : std::filesystem::directory_iterator(R"(../../test/skill_ready/c)")) {
        cv::Mat image = MAA_NS::imread(entry.path());
        BattlefieldClassifier analyzer(image);
        analyzer.set_object_of_interest({ .skill_ready = true });
        total++;
        auto result = analyzer.analyze()->skill_ready;
        Log.info(
            __FUNCTION__,
            "File: ",
            entry.path().string(),
            " | Expected: C (ready: false)",
            " | Predicted: ",
            result.ready,
            " | Score: ",
            result.score,
            " | Prob: ",
            result.prob);
        if (!result.ready) {
            correct++;
        }
    }

    Log.info(__FUNCTION__, "Final Accuracy: ", correct, "/", total, " (", double(correct) / total, ")");
}

void asst::DebugTask::test_battle_image()
{
    cv::Mat image = MAA_NS::imread(utils::path("1.png"));
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(1280, 720), 0, 0, cv::INTER_AREA);
    BattlefieldMatcher analyzer(resized);
    analyzer.set_object_of_interest({ .deployment = true });
    analyzer.analyze();
}

void asst::DebugTask::test_match_template()
{
    auto test_task = [](const std::string& path, const std::string& task_name) -> double {
        cv::Mat image = MAA_NS::imread(utils::path(path));
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
            return result_opt.value().score;
        }
        else {
            Log.info("inactive", path, task_name);
            return 0.;
        }
    };

    // test_task(
    //     "../../x64/Release/debug/roguelike/2024-07-27_16-32-25-198_raw.png",
    //     "Sarkaz@Roguelike@StageWindAndRain");

    // for (int i = 1; i <= 15; ++i) {
    //     test_task("../../test/dist/" + std::to_string(i) + ".png", "Sarkaz@Roguelike@StageCombatOps");
    //     test_task("../../test/dist/" + std::to_string(i) + ".png", "Sarkaz@Roguelike@StageBoskyPassage");
    //     test_task("../../test/dist/" + std::to_string(i) + ".png", "Sarkaz@Roguelike@StageEmergencyTransportation");
    //     test_task("../../test/dist/" + std::to_string(i) + ".png", "Sarkaz@Roguelike@StageWindAndRain");
    // }

#define TEST(expr)                                       \
    if (!(expr)) {                                       \
        throw std::runtime_error("Test failed: " #expr); \
    }

#define ASSERT_ACTIVE(path, task_name) TEST(test_task(path, task_name) > DoubleDiff)
#define ASSERT_INACTIVE(path, task_name) TEST(test_task(path, task_name) < DoubleDiff)

    ASSERT_INACTIVE("../../test/dist/12.png", "Sarkaz@Roguelike@StageBoskyPassage");
    ASSERT_ACTIVE("../../test/dist/13.png", "Sarkaz@Roguelike@StageEmergencyTransportation");
    ASSERT_ACTIVE("../../test/dist/14.png", "Sarkaz@Roguelike@StageWindAndRain");
    ASSERT_ACTIVE("../../test/dist/15.png", "Sarkaz@Roguelike@StageEmergencyTransportation");
    ASSERT_ACTIVE("../../test/dist/#10160.png", "Sarkaz@Roguelike@StageTraderEnter");
    ASSERT_INACTIVE("../../test/dist/#10235.png", "Sarkaz@Roguelike@StageRefresh");

#undef TEST
#undef ASSERT_ACTIVE
#undef ASSERT_INACTIVE
}
