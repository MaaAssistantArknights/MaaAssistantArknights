#include "BattlefieldClassifier.h"

#include "MaaUtils/NoWarningCV.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <format>
#include <map>
#include <mutex>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "Config/OnnxSessions.h"
#include "Config/TaskData.h"
#include "MaaUtils/ImageIo.h"
#include "Utils/Logger.hpp"

using namespace asst;

BattlefieldClassifier::ResultOpt BattlefieldClassifier::analyze() const
{
    Result result { .object_of_interest = m_object_of_interest };
    bool analyzed = false;

    if (m_object_of_interest.skill_ready) {
        result.skill_ready = skill_ready_analyze();
        analyzed = true;
    }

    if (m_object_of_interest.deploy_direction) {
        result.deploy_direction = deploy_direction_analyze();
        analyzed = true;
    }

    if (!analyzed) {
        return std::nullopt;
    }

    return result;
}

BattlefieldClassifier::SkillReadyResult BattlefieldClassifier::skill_ready_analyze() const
{
    auto task_ptr = Task.get<MatchTaskInfo>("BattleSkillReady");
    const Rect& skill_roi_move = task_ptr->rect_move;
    Rect roi = Rect(m_base_point.x, m_base_point.y, 0, 0).move(skill_roi_move);

    cv::Mat image = make_roi(m_image, correct_rect(roi, m_image));

    // 1. 图像大小调整(推理慢可不做)
    cv::Mat resized_image;
    cv::resize(image, resized_image, cv::Size(72, 72), 0.0, 0.0, cv::INTER_CUBIC);

    // 2. 中心裁剪(推理慢可不做)
    int crop_size = 64;
    int x = (resized_image.cols - crop_size) / 2;
    int y = (resized_image.rows - crop_size) / 2;
    cv::Rect crop_roi(x, y, crop_size, crop_size);
    cv::Mat cropped_image = resized_image(crop_roi);

    // 3. 图像转换为 tensor
    std::vector<float> input = image_to_tensor(cropped_image);

    // 4. 归一化
    static constexpr std::array<float, 3> kMean { 0.485f, 0.456f, 0.406f };
    static constexpr std::array<float, 3> kStd { 0.229f, 0.224f, 0.225f };
    const size_t plane_size = static_cast<size_t>(cropped_image.rows) * static_cast<size_t>(cropped_image.cols);

    for (size_t channel = 0; channel < 3; ++channel) {
        const size_t offset = channel * plane_size;
        for (size_t index = 0; index < plane_size; ++index) {
            input[offset + index] = (input[offset + index] - kMean[channel]) / kStd[channel];
        }
    }

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
    constexpr int64_t batch_size = 1;

    auto& session = OnnxSessions::get_instance().get("skill_ready_cls");

    std::array<int64_t, 4> input_shape { batch_size, cropped_image.channels(), cropped_image.rows, cropped_image.cols };

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        input.data(),
        input.size(),
        input_shape.data(),
        input_shape.size());

    SkillReadyResult::Raw raw_results;
    std::array<int64_t, 2> output_shape { batch_size, SkillReadyResult::ClsSize };
    Ort::Value output_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        raw_results.data(),
        raw_results.size(),
        output_shape.data(),
        output_shape.size());

    // 这俩是hardcode在模型里的
    constexpr const char* input_names[] = { "input" };   // session.GetInputName()
    constexpr const char* output_names[] = { "output" }; // session.GetOutputName()

    Ort::RunOptions run_options;
    session.Run(run_options, input_names, &input_tensor, 1, output_names, &output_tensor, 1);
    Log.info(__FUNCTION__, "raw results:", raw_results);

    SkillReadyResult::Prob prob = softmax(raw_results);
    Log.info(__FUNCTION__, "prob:", prob);
    // 类别顺序为 c, n, y
    int class_id = static_cast<int>(std::ranges::max_element(prob) - prob.begin());
    bool ready = class_id == 2; // 只有当class_id为2（代表y）时，才认为是ready
    float score = prob[class_id];

#ifdef ASST_DEBUG
    // 在调试模式下，根据不同类别绘制不同颜色的标记
    if (class_id == 2) {
        // y类别：橙色
        rectangle(m_image_draw, make_rect<cv::Rect>(roi), cv::Scalar(0, 165, 255), 2);
        putText(m_image_draw, std::to_string(score), cv::Point(roi.x, roi.y - 10), 1, 1.2, cv::Scalar(0, 165, 255), 2);
    }
    else if (class_id == 0) { // c类别的特殊处理
        // 使用蓝色（BGR：255,0,0）标记c类别
        rectangle(m_image_draw, make_rect<cv::Rect>(roi), cv::Scalar(255, 0, 0), 2);
        putText(m_image_draw, std::to_string(score), cv::Point(roi.x, roi.y - 10), 1, 1.2, cv::Scalar(255, 0, 0), 2);
    }
#endif

    const auto result = SkillReadyResult {
        .ready = ready,
        .rect = roi,
        .score = score,
        .raw = raw_results,
        .prob = prob,
        .base_point = m_base_point,
    };

    static const bool save_infinitely = std::filesystem::exists("DEBUG_skill_ready.txt");

    // 为重新训练模型截图
    struct point_state
    {
        int last_class = -1;
        std::chrono::steady_clock::time_point last_save_time;
    };

    static std::unordered_map<Point, point_state> point_states;
    static std::mutex point_states_mutex;

    const auto now = std::chrono::steady_clock::now();

    bool need_save = false;

    {
        std::lock_guard<std::mutex> lock(point_states_mutex);
        auto& [last_class, last_save_time] = point_states[m_base_point];
        const auto duration_since_last_save =
            std::chrono::duration_cast<std::chrono::seconds>(now - last_save_time).count();

        // 判断当前类别是否与上次保存的类别不同
        if (last_class != class_id) {
            Log.trace("Class changed", last_class, class_id);
            need_save = true;
        }
        // y 1 秒存一次（最小开技能间隔为 1.5s），c 5 秒存一次
        else if ((class_id == 2 && duration_since_last_save > 1) || (class_id == 0 && duration_since_last_save > 5)) {
            Log.trace("Class is", class_id);
            need_save = true;
        }
        // 长时间没变化，可能是被遮挡了
        else if (duration_since_last_save > 10) {
            Log.trace("Long time no change", duration_since_last_save);
            need_save = true;
        }

        // 新增：如果最高得分低于阈值，则保存
        if (score < 0.75f && duration_since_last_save > 1) {
            Log.trace("Low score", score);
            need_save = true;
        }

        if (need_save) {
            last_class = class_id;
            last_save_time = now;
        }
    }

    if (need_save) {
        std::string subfolder;
        switch (class_id) {
        case 2:
            subfolder = "y";
            break;
        case 1:
            subfolder = "n";
            break;
        case 0:
            subfolder = "c";
            break;
        default:
            subfolder = "unknown";
            break;
        }

        std::string filename = std::format(
            "{}_{}_{}(c{:3f})(n{:3f})(y{:3f}).png",
            MAA_NS::format_now_for_filename(),
            m_base_point.x,
            m_base_point.y,
            prob[0],
            prob[1],
            prob[2]);

        save_skill_ready_debug_image(image, subfolder, filename, !save_infinitely);
    }

    return result;
}

BattlefieldClassifier::DeployDirectionResult BattlefieldClassifier::deploy_direction_analyze() const
{
    const auto& task_ptr = Task.get<MatchTaskInfo>("BattleDeployDirectionRectMove");
    const Rect& roi_move = task_ptr->rect_move;
    Rect roi = Rect(m_base_point.x, m_base_point.y, 0, 0).move(roi_move);

    cv::Mat image = make_roi(m_image, correct_rect(roi, m_image));
    std::vector<float> input = image_to_tensor(image);

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
    constexpr int64_t batch_size = 1;
    std::array<int64_t, 4> input_shape { batch_size, image.channels(), image.cols, image.rows };

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        input.data(),
        input.size(),
        input_shape.data(),
        input_shape.size());

    DeployDirectionResult::Raw raw_results;
    std::array<int64_t, 2> output_shape { batch_size, DeployDirectionResult::ClsSize };
    Ort::Value output_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        raw_results.data(),
        raw_results.size(),
        output_shape.data(),
        output_shape.size());

    auto& session = OnnxSessions::get_instance().get("deploy_direction_cls");
    // 这俩是 hardcode 在模型里的
    constexpr const char* input_names[] = { "input" };   // session.GetInputName()
    constexpr const char* output_names[] = { "output" }; // session.GetOutputName()

    Ort::RunOptions run_options;
    session.Run(run_options, input_names, &input_tensor, 1, output_names, &output_tensor, 1);
    Log.info(__FUNCTION__, "raw result:", raw_results);

    DeployDirectionResult::Prob prob = softmax(raw_results);
    Log.info(__FUNCTION__, "after softmax:", prob);

    size_t class_id = std::max_element(prob.begin(), prob.end()) - prob.begin();

#ifdef ASST_DEBUG
    static const std::unordered_map<size_t, std::string> ClassNames = {
        { 0, "Right" },
        { 1, "Down" },
        { 2, "Left" },
        { 3, "Up" },
    };
    if (ClassNames.size() != prob.size()) {
        Log.error("ClassNames.size() != prob.size()", ClassNames.size(), prob.size());
        throw std::runtime_error("ClassNames.size() != prob.size()");
    }
    cv::putText(
        m_image_draw,
        ClassNames.at(class_id),
        cv::Point(roi.x, roi.y + roi.height),
        cv::FONT_HERSHEY_PLAIN,
        1.2,
        cv::Scalar(0, 255, 0),
        2);
    cv::putText(
        m_image_draw,
        std::to_string(prob[class_id]),
        cv::Point(roi.x, roi.y + roi.height + 20),
        cv::FONT_HERSHEY_PLAIN,
        1.2,
        cv::Scalar(0, 255, 0),
        2);
#endif

    return DeployDirectionResult {
        .direction = static_cast<battle::DeployDirection>(class_id),
        .rect = roi,
        .score = prob[class_id],
        .raw = raw_results,
        .prob = prob,
        .base_point = m_base_point,
    };
}

void BattlefieldClassifier::init_skill_ready_file_queue_locked(
    const std::filesystem::path& dir,
    SkillReadyFileQueue& file_queue)
{
    if (file_queue.initialized) {
        return;
    }
    file_queue.initialized = true;

    std::error_code dir_ec;
    if (!std::filesystem::is_directory(dir, dir_ec)) {
        if (dir_ec) {
            Log.warn(__FUNCTION__, "failed to inspect image directory", dir, dir_ec.message());
        }
        return;
    }

    std::vector<std::pair<std::filesystem::file_time_type, std::filesystem::path>> files;
    std::error_code iter_ec;
    const auto options = std::filesystem::directory_options::skip_permission_denied;
    for (std::filesystem::directory_iterator iter(dir, options, iter_ec), end; iter != end; iter.increment(iter_ec)) {
        if (iter_ec) {
            Log.warn(__FUNCTION__, "failed to iterate image directory", dir, iter_ec.message());
            break;
        }

        const auto& entry = *iter;
        std::error_code entry_ec;
        if (!entry.is_regular_file(entry_ec)) {
            if (entry_ec) {
                Log.warn(__FUNCTION__, "failed to inspect image entry", entry.path(), entry_ec.message());
            }
            continue;
        }

        const auto path = entry.path();
        const auto write_time = std::filesystem::last_write_time(path, entry_ec);
        if (entry_ec) {
            Log.warn(__FUNCTION__, "failed to query image timestamp", path, entry_ec.message());
            continue;
        }

        files.emplace_back(write_time, path);
    }

    std::sort(files.begin(), files.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.first != rhs.first) {
            return lhs.first < rhs.first;
        }
        return lhs.second < rhs.second;
    });

    const std::size_t excess = files.size() > SkillReadyAutoCleanLimit ? files.size() - SkillReadyAutoCleanLimit : 0;
    for (std::size_t i = 0; i < excess; ++i) {
        std::error_code ec;
        std::filesystem::remove(files[i].second, ec);
        if (ec) {
            Log.warn(__FUNCTION__, "failed to remove old image", files[i].second, ec.message());
        }
    }

    for (std::size_t i = excess; i < files.size(); ++i) {
        file_queue.files.emplace_back(std::move(files[i].second));
    }
}

bool BattlefieldClassifier::save_skill_ready_debug_image(
    const cv::Mat& image,
    const std::string& subfolder,
    const std::string& filename,
    bool auto_clean)
{
    if (image.empty()) {
        return false;
    }

    const auto relative_dir = utils::path("debug") / "skill_ready" / utils::path(std::string(subfolder));
    const auto absolute_dir = (UserDir.get() / relative_dir).lexically_normal();
    const auto absolute_path = absolute_dir / utils::path(filename);

    std::error_code create_ec;
    std::filesystem::create_directories(absolute_dir, create_ec);
    if (create_ec) {
        Log.warn(__FUNCTION__, "failed to create image directory", absolute_dir, create_ec.message());
        return false;
    }

    if (auto_clean) {
        static std::map<std::filesystem::path, SkillReadyFileQueue> s_file_queues;
        static std::mutex s_mutex;

        std::filesystem::path old_path;
        bool remove_old_path = false;

        {
            std::lock_guard<std::mutex> lock(s_mutex);
            auto& file_queue = s_file_queues[absolute_dir];
            init_skill_ready_file_queue_locked(absolute_dir, file_queue);

            if (file_queue.files.size() >= SkillReadyAutoCleanLimit) {
                old_path = std::move(file_queue.files.front());
                file_queue.files.pop_front();
                remove_old_path = true;
            }

            file_queue.files.emplace_back(absolute_path);
        }

        if (remove_old_path) {
            std::error_code ec;
            std::filesystem::remove(old_path, ec);
            if (ec) {
                Log.warn(__FUNCTION__, "failed to remove old image", old_path, ec.message());
            }
        }

        Log.trace("Save image", absolute_path);
        if (!MAA_NS::imwrite(absolute_path, image)) {
            std::lock_guard<std::mutex> lock(s_mutex);
            auto queue_iter = s_file_queues.find(absolute_dir);
            if (queue_iter != s_file_queues.end()) {
                auto& files = queue_iter->second.files;
                for (auto iter = files.end(); iter != files.begin();) {
                    --iter;
                    if (*iter == absolute_path) {
                        files.erase(iter);
                        break;
                    }
                }
            }
            return false;
        }

        return true;
    }

    Log.trace("Save image", absolute_path);
    return MAA_NS::imwrite(absolute_path, image);
}
