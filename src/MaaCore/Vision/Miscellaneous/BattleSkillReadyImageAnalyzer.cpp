#include "BattleSkillReadyImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <onnxruntime/core/session/onnxruntime_cxx_api.h>

#include "Config/OnnxSession.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"

template <typename T>
static void softmax(T& input)
{
    float rowmax = *std::max_element(input.begin(), input.end());
    std::vector<float> y(input.size());
    float sum = 0.0f;
    for (size_t i = 0; i != input.size(); ++i) {
        sum += y[i] = std::exp(input[i] - rowmax);
    }
    for (size_t i = 0; i != input.size(); ++i) {
        input[i] = y[i] / sum;
    }
}

bool asst::BattleSkillReadyImageAnalyzer::analyze()
{
    cv::Mat image = m_image(make_rect<cv::Rect>(m_roi)).clone();
    cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

    size_t input_size = 1ULL * image.cols * image.rows * image.channels();
    std::vector<float> input(input_size);
    std::vector<float> output;
    size_t count = 0;
    for (int k = 0; k < image.channels(); k++) {
        for (int i = 0; i < image.rows; i++) {
            for (int j = 0; j < image.cols; j++) {
                float value = static_cast<float>(image.at<cv::Vec3b>(i, j)[k]);
                value /= 255;
                input[count++] = value;
            }
        }
    }

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
    constexpr int64_t batch_size = 1;
    std::array<int64_t, 4> input_shape { batch_size, image.channels(), image.cols, image.rows };

    Ort::Value input_tensor =
        Ort::Value::CreateTensor<float>(memory_info, input.data(), input_size, input_shape.data(), input_shape.size());

    constexpr size_t classification_size = 2;
    std::array<float, classification_size> results;
    std::array<int64_t, 2> output_shape { batch_size, classification_size };
    Ort::Value output_tensor = Ort::Value::CreateTensor<float>(memory_info, results.data(), results.size(),
                                                               output_shape.data(), output_shape.size());

    auto& session = OnnxSession::get_instance().get("skill_ready_rec");
    // 这俩是hardcode在模型里的
    constexpr const char* input_names[] = { "input" };   // session.GetInputName()
    constexpr const char* output_names[] = { "output" }; // session.GetOutputName()

    Ort::RunOptions run_options;
    session.Run(run_options, input_names, &input_tensor, 1, output_names, &output_tensor, 1);
    Log.info(__FUNCTION__, "raw result, 0: ", results[0], ", 1: ", results[1]);

    softmax(results);
    Log.info(__FUNCTION__, "after softmax, 0: ", results[0], ", 1: ", results[1]);
    return results[1] > results[0];
}

void asst::BattleSkillReadyImageAnalyzer::set_base_point(const Point& pt)
{
    auto task_ptr = Task.get<MatchTaskInfo>("BattleSkillReady");
    const Rect& skill_roi_move = task_ptr->rect_move;

    set_roi(Rect(pt.x, pt.y, 0, 0).move(skill_roi_move));
}
