#include "DebugTask.h"

#include "MaaUtils/Encoding.h"
#include <boost/regex.hpp>
#include <filesystem>
#include <shared_mutex>

#include "Common/AsstMsg.h"
#include "Common/AsstTypes.h"
#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Battle/BattlefieldClassifier.h"
#include "Vision/Battle/BattlefieldMatcher.h"
#include "Vision/BestMatcher.h"
#include "Vision/FeatureMatcher.h"
#include "Vision/Matcher.h"
#include "Vision/Miscellaneous/DepotImageAnalyzer.h"
#include "Vision/Miscellaneous/PipelineAnalyzer.h"
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
    std::string test = "[1,2,3,4]";
    json::value test_json = json::parse(test);
    LogInfo << "DebugTask run() called, test_json:" << test_json;
    LogInfo << "DebugTask run() called, test_json:" << test_json.is<asst::Rect>();

    if (m_image_test_mode == "report") {
        return image_test_report();
    }
    if (m_image_test_mode == "pipeline") {
        return image_test_pipeline();
    }
    if (m_image_test_mode == "ocr") {
        return image_test_ocr();
    }
    if (m_image_test_mode == "templ") {
        return image_test_templ();
    }
    return true;
}

namespace
{
// 读取本地图片；normalize 为 true 时缩放到 1280x720（INTER_AREA），
// 与线上 Controller::get_resized_image_cache 一致，调用方已自行预处理尺寸时传 false
std::optional<cv::Mat> load_eval_image(const std::string& utf8_path, bool normalize)
{
    cv::Mat image = MAA_NS::imread(asst::utils::path(utf8_path));
    if (image.empty()) {
        LogError << __FUNCTION__ << "failed to load image:" << utf8_path;
        return std::nullopt;
    }
    if (image.cols * 9 != image.rows * 16) {
        // 线上遇到非 16:9 截图会直接报 Unsupported resolution，这里拉伸后仍可评估，
        // 但结果与该分辨率的线上行为没有可比性
        LogWarn << __FUNCTION__ << "image is not 16:9, will be stretched to 1280x720:" << utf8_path;
    }
    if (normalize && (image.cols != 1280 || image.rows != 720)) {
        cv::resize(image, image, { 1280, 720 }, 0, 0, cv::INTER_AREA);
    }
    return image;
}

// 可选的评估前缩放（INTER_AREA），供调用方复刻线上各识别器的尺度预处理
std::optional<cv::Mat> resize_eval_image(cv::Mat image, int width, int height)
{
    if (image.cols != width || image.rows != height) {
        cv::resize(image, image, { width, height }, 0, 0, cv::INTER_AREA);
    }
    return image;
}

json::object to_result_json(const std::string& task_name, const asst::PipelineAnalyzer::ResultOpt& result_opt)
{
    json::object result { { "task", task_name } };
    if (!result_opt) {
        result["hit"] = false;
        return result;
    }
    result["hit"] = true;
    // JustReturn 任务无识别结果（variant 默认构造的 Matcher::Result），带上算法类型供调用方区分
    result["algorithm"] = asst::enum_to_string(result_opt->task_ptr->algorithm);
    const auto& result_var = result_opt->result;
    if (std::holds_alternative<asst::Matcher::Result>(result_var)) {
        const auto& r = std::get<asst::Matcher::Result>(result_var);
        result["score"] = r.score;
        result["templ"] = r.templ_name;
    }
    else if (std::holds_alternative<asst::OCRer::Result>(result_var)) {
        const auto& r = std::get<asst::OCRer::Result>(result_var);
        result["score"] = r.score;
        result["text"] = r.text;
    }
    else if (std::holds_alternative<asst::FeatureMatcher::Result>(result_var)) {
        // FeatureMatch 的判据是特征点数而非相似度分数
        const auto& r = std::get<asst::FeatureMatcher::Result>(result_var);
        result["count"] = r.count;
    }
    result["rect"] = (json::value)result_opt->rect;
    return result;
}
}

// 离线图片评估的参数协议（AsstAppendTask 的 params，type 固定为 "Debug"）。
// 日常用配套的 python 驱动 tools/maa_core_eval.py 调用，无需手拼 json：
// { "mode": "report" | "pipeline" | "ocr" | "templ", "images": [图片路径], "tasks": [任务名],
//   "templates": [模板名], "task": "任务名", "roi": [x, y, w, h], "threshold": 0.8, "resize": [w, h] }
// images/tasks/templates 为 UTF-8 路径与名字（模板名也接受绝对路径图片文件）；roi 仅 ocr/templ
// 模式使用，缺省全图；task 仅 templ 模式使用（mask/method 等 Matcher 配置取自该任务，复刻线上
// 自定义识别器的用法，未显式给 threshold 时 hit 判定也取该任务阈值）；threshold/resize 仅 templ
// 模式使用（内部匹配放开阈值恒报最佳得分；resize 为评估前 INTER_AREA 缩放尺寸，替代默认的
// 1280x720 归一）
bool asst::DebugTask::set_params(const json::value& params)
{
    LogTraceFunction;

    // AsstSetTaskParams 在任务运行中调用会与 run() 遍历 m_eval_* 产生数据竞争，运行中拒绝更新
    if (m_running) {
        LogError << __FUNCTION__ << "failed, task is running";
        return false;
    }

    // 畸形参数（如 roi/resize 数组里有非数字）会让 as_integer 抛异常，不能让它穿过 C 接口
    try {
        return set_params_impl(params);
    }
    catch (const std::exception& e) {
        LogError << __FUNCTION__ << "failed, invalid params:" << e.what();
        return false;
    }
}

bool asst::DebugTask::set_params_impl(const json::value& params)
{
    m_image_test_mode = params.get("mode", "");
    if (m_image_test_mode.empty()) {
        return true; // 无 mode 的 Debug 任务保持 run() 空跑的旧行为
    }

    // AsstSetTaskParams 会对同一任务重复调用 set_params，先清空上次状态避免累积
    m_eval_images.clear();
    m_eval_tasks.clear();
    m_eval_templates.clear();
    m_eval_templ_task.clear();
    m_eval_roi = Rect();
    m_eval_threshold = 0.8;
    m_eval_resize.reset();

    auto images_opt = params.find<std::vector<std::string>>("images");
    if (!images_opt || images_opt->empty()) {
        LogError << __FUNCTION__ << "failed, images not found";
        return false;
    }
    for (auto& image : *images_opt) {
        m_eval_images.emplace_back(std::move(image));
    }

    if (m_image_test_mode == "report" || m_image_test_mode == "pipeline") {
        auto tasks_opt = params.find<std::vector<std::string>>("tasks");
        if (!tasks_opt || tasks_opt->empty()) {
            LogError << __FUNCTION__ << "failed, tasks not found";
            return false;
        }
        for (auto& task : *tasks_opt) {
            if (Task.get(task) == nullptr) {
                LogError << __FUNCTION__ << "failed, task not found:" << task;
                return false;
            }
            m_eval_tasks.emplace_back(std::move(task));
        }
    }
    else if (m_image_test_mode == "ocr" || m_image_test_mode == "templ") {
        if (auto roi_opt = params.find<asst::Rect>("roi"); roi_opt) {
            m_eval_roi = *roi_opt;
        }
        if (m_image_test_mode == "templ") {
            auto templates_opt = params.find<std::vector<std::string>>("templates");
            if (!templates_opt || templates_opt->empty()) {
                LogError << __FUNCTION__ << "failed, templates not found";
                return false;
            }
            for (const auto& templ : *templates_opt) {
                m_eval_templates.emplace_back(templ);
            }
            if (auto task_opt = params.find<std::string>("task"); task_opt) {
                // Matcher 配置只能取自模板类任务；Task.get<MatchTaskInfo> 对非 MatchTaskInfo
                // 任务返回空，后续 set_task_info 会对其解引用，必须在入口拒绝
                auto match_ptr = Task.get<MatchTaskInfo>(*task_opt);
                if (match_ptr == nullptr) {
                    LogError << "set_params failed, task not found or not a match task:" << *task_opt;
                    return false;
                }
                m_eval_templ_task = *task_opt;
                double default_threshold =
                    !match_ptr->templ_thresholds.empty() ? match_ptr->templ_thresholds.front() : 0.8;
                m_eval_threshold = params.get("threshold", default_threshold);
            }
            else {
                m_eval_threshold = params.get("threshold", 0.8);
            }
            if (auto resize_opt = params.find<std::array<int, 2>>("resize"); resize_opt) {
                int resize_w = (*resize_opt)[0];
                int resize_h = (*resize_opt)[1];
                if (resize_w <= 0 || resize_h <= 0) {
                    LogError << "set_params failed, invalid resize:" << resize_w << resize_h;
                    return false;
                }
                m_eval_resize = std::make_pair(resize_w, resize_h);
            }
        }
    }
    else {
        LogError << "set_params failed, unknown mode:" << m_image_test_mode;
        return false;
    }

    return true;
}

void asst::DebugTask::emit_eval_error(const std::string& mode, const std::string& image_path, const std::string& error)
{
    LogError << __FUNCTION__ << image_path << error;
    callback(
        AsstMsg::SubTaskExtraInfo,
        json::object { { "what", "DebugImageTest" },
                       { "details", json::object { { "mode", mode }, { "image", image_path }, { "error", error } } } });
}

bool asst::DebugTask::image_test_report()
{
    bool all_ok = true;
    for (const auto& image_path : m_eval_images) {
        try {
            auto image_opt = load_eval_image(image_path, true);
            if (!image_opt) {
                all_ok = false;
                emit_eval_error("report", image_path, "failed to load image");
                continue;
            }

            json::array results;
            for (const auto& task_name : m_eval_tasks) {
                PipelineAnalyzer analyzer(*image_opt, Rect(0, 0, 1280, 720), nullptr);
                analyzer.set_tasks({ task_name });
                auto result_opt = analyzer.analyze();

                json::object result = to_result_json(task_name, result_opt);
                LogInfo << __FUNCTION__ << image_path << task_name << (result_opt ? "hit" : "miss") << result.dumps();
                results.emplace_back(std::move(result));
            }

            callback(
                AsstMsg::SubTaskExtraInfo,
                json::object { { "what", "DebugImageTest" },
                               { "details",
                                 json::object { { "mode", "report" },
                                                { "image", image_path },
                                                { "results", std::move(results) } } } });
        }
        catch (const std::exception& e) {
            // ASST_DEBUG 下模板缺失/为空等会 throw，逐图兜住避免后续图静默缺结果
            all_ok = false;
            emit_eval_error("report", image_path, e.what());
        }
    }
    return all_ok;
}

bool asst::DebugTask::image_test_pipeline()
{
    bool all_ok = true;
    for (const auto& image_path : m_eval_images) {
        try {
            auto image_opt = load_eval_image(image_path, true);
            if (!image_opt) {
                all_ok = false;
                emit_eval_error("pipeline", image_path, "failed to load image");
                continue;
            }

            PipelineAnalyzer analyzer(*image_opt, Rect(0, 0, 1280, 720), nullptr);
            analyzer.set_tasks(m_eval_tasks);
            auto result_opt = analyzer.analyze();

            json::object detail { { "mode", "pipeline" }, { "image", image_path } };
            json::array next;
            if (result_opt) {
                const auto hit = to_result_json(result_opt->task_ptr->name, result_opt);
                for (const auto& [key, value] : hit) {
                    detail[key] = value;
                }
                for (const auto& next_task : result_opt->task_ptr->next) {
                    next.emplace_back(next_task);
                }
                LogInfo << __FUNCTION__ << image_path << "hit" << hit.dumps();
            }
            else {
                detail["hit"] = false;
                LogInfo << __FUNCTION__ << image_path << "miss";
            }
            detail["next"] = std::move(next);

            callback(
                AsstMsg::SubTaskExtraInfo,
                json::object { { "what", "DebugImageTest" }, { "details", std::move(detail) } });
        }
        catch (const std::exception& e) {
            all_ok = false;
            emit_eval_error("pipeline", image_path, e.what());
        }
    }
    return all_ok;
}

bool asst::DebugTask::image_test_ocr()
{
    bool all_ok = true;
    for (const auto& image_path : m_eval_images) {
        try {
            auto image_opt = load_eval_image(image_path, true);
            if (!image_opt) {
                all_ok = false;
                emit_eval_error("ocr", image_path, "failed to load image");
                continue;
            }

            // 不 set_task_info：默认参数下 required 为空即不做 expected 过滤、不做 ocrReplace，
            // 返回 OCR 引擎的原始识别结果；带任务配置的评估用 report 模式
            Rect roi = m_eval_roi.empty() ? Rect(0, 0, 1280, 720) : m_eval_roi;
            OCRer analyzer(*image_opt, roi);
            auto results_opt = analyzer.analyze();

            json::array results;
            if (results_opt) {
                for (const auto& res : *results_opt) {
                    results.emplace_back(
                        json::object { { "text", res.text },
                                       { "score", res.score },
                                       { "rect", (json::value)(res.rect) } });
                }
            }
            LogInfo << __FUNCTION__ << image_path << "ocr" << results.dumps();

            callback(
                AsstMsg::SubTaskExtraInfo,
                json::object { { "what", "DebugImageTest" },
                               { "details",
                                 json::object { { "mode", "ocr" },
                                                { "image", image_path },
                                                { "results", std::move(results) } } } });
        }
        catch (const std::exception& e) {
            all_ok = false;
            emit_eval_error("ocr", image_path, e.what());
        }
    }
    return all_ok;
}

bool asst::DebugTask::image_test_templ()
{
    bool all_ok = true;
    for (const auto& image_path : m_eval_images) {
        try {
            auto image_opt = load_eval_image(image_path, !m_eval_resize.has_value());
            if (image_opt && m_eval_resize) {
                image_opt = resize_eval_image(std::move(*image_opt), m_eval_resize->first, m_eval_resize->second);
            }
            if (!image_opt) {
                all_ok = false;
                emit_eval_error("templ", image_path, "failed to load image");
                continue;
            }

            json::array results;
            for (const auto& templ_name : m_eval_templates) {
                // 模板名与 core 各处 get_templ 一致：物品 ID（如 "2001"）或相对
                // resource/template 的路径（如 "items/2001.png"）；也接受绝对路径的
                // 图片文件（调用方自行预处理过的模板）
                Matcher analyzer(*image_opt, Rect(0, 0, image_opt->cols, image_opt->rows), nullptr);

                if (!m_eval_templ_task.empty()) {
                    // mask/method 等 Matcher 配置取自该任务；须在 set_templ 之前调用，
                    // 否则 set_task_info 会把模板重置为任务自带的
                    analyzer.set_task_info(m_eval_templ_task);
                }
                if (!m_eval_roi.empty()) {
                    // set_task_info 会用任务自身的 roi 覆盖构造时的 roi，显式传入的 roi 在其后重设
                    analyzer.set_roi(m_eval_roi);
                }

                json::object result { { "template", templ_name } };
                std::filesystem::path templ_file = asst::utils::path(templ_name);
                if (templ_file.is_absolute() && std::filesystem::exists(templ_file)) {
                    cv::Mat templ = MAA_NS::imread(templ_file);
                    if (templ.empty()) {
                        result["hit"] = false;
                        result["error"] = "failed to load templ file";
                        LogError << __FUNCTION__ << "failed to load templ:" << templ_name;
                        all_ok = false;
                        results.emplace_back(std::move(result));
                        continue;
                    }
                    analyzer.set_templ(std::move(templ));
                }
                else {
                    analyzer.set_templ(templ_name);
                }
                analyzer.set_threshold(-1.0); // 放开阈值恒报最佳得分，hit 由 threshold 字段判定

                try {
                    auto result_opt = analyzer.analyze();
                    if (result_opt) {
                        result["score"] = result_opt->score;
                        result["rect"] = (json::value)(result_opt->rect);
                        result["hit"] = result_opt->score >= m_eval_threshold;
                    }
                    else {
                        // 阈值已放开仍无结果，只剩 roi 为空或模板大于 roi 等输入问题，不存在正常 miss
                        result["hit"] = false;
                        result["error"] = "no match result (check roi / template size)";
                        all_ok = false;
                    }
                }
                catch (const std::exception& e) {
                    // ASST_DEBUG 下模板不存在/加载失败会 throw，报错后继续评估其余模板
                    result["hit"] = false;
                    result["error"] = e.what();
                    all_ok = false;
                }
                LogInfo << __FUNCTION__ << image_path << templ_name << (result["hit"].as_boolean() ? "hit" : "miss")
                        << result.dumps();
                results.emplace_back(std::move(result));
            }

            callback(
                AsstMsg::SubTaskExtraInfo,
                json::object { { "what", "DebugImageTest" },
                               { "details",
                                 json::object { { "mode", "templ" },
                                                { "image", image_path },
                                                { "results", std::move(results) } } } });
        }
        catch (const std::exception& e) {
            all_ok = false;
            emit_eval_error("templ", image_path, e.what());
        }
    }
    return all_ok;
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
