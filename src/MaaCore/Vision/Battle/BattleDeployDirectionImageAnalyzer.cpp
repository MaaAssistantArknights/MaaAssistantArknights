#include "BattleDeployDirectionImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <onnxruntime/core/session/onnxruntime_cxx_api.h>

#include "Config/OnnxSessions.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"

bool asst::BattleDeployDirectionImageAnalyzer::analyze()
{
    cv::Mat image = m_image(make_rect<cv::Rect>(m_roi));
    std::vector<float> input = image_to_tensor(image);

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
    constexpr int64_t batch_size = 1;
    std::array<int64_t, 4> input_shape { batch_size, image.channels(), image.cols, image.rows };

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input.data(), input.size(),
                                                              input_shape.data(), input_shape.size());

    RawResults results;
    std::array<int64_t, 2> output_shape { batch_size, ClassificationSize };
    Ort::Value output_tensor = Ort::Value::CreateTensor<float>(memory_info, results.data(), results.size(),
                                                               output_shape.data(), output_shape.size());

    auto& session = OnnxSessions::get_instance().get("deploy_direction_cls");
    // 这俩是hardcode在模型里的
    constexpr const char* input_names[] = { "input" };   // session.GetInputName()
    constexpr const char* output_names[] = { "output" }; // session.GetOutputName()

    Ort::RunOptions run_options;
    session.Run(run_options, input_names, &input_tensor, 1, output_names, &output_tensor, 1);
    m_raw_results = results;
    Log.info(__FUNCTION__, "raw result:", m_raw_results);

    softmax(results);
    Log.info(__FUNCTION__, "after softmax:", results);

    m_class_id = std::max_element(results.begin(), results.end()) - results.begin();

#ifdef ASST_DEBUG
    static const std::unordered_map<size_t, std::string> ClassNames = {
        { 0, "Right" },
        { 1, "Down" },
        { 2, "Left" },
        { 3, "Up" },
    };
    cv::putText(m_image_draw, ClassNames.at(m_class_id), cv::Point(m_roi.x, m_roi.y + m_roi.height),
                cv::FONT_HERSHEY_PLAIN, 1.2, cv::Scalar(0, 255, 0), 2);
    cv::putText(m_image_draw, std::to_string(results[m_class_id]), cv::Point(m_roi.x, m_roi.y + m_roi.height + 20),
                cv::FONT_HERSHEY_PLAIN, 1.2, cv::Scalar(0, 255, 0), 2);
#endif

    return true;
}

void asst::BattleDeployDirectionImageAnalyzer::set_base_point(const Point& pt)
{
    auto task_ptr = Task.get<MatchTaskInfo>("BattleDeployDirectionRectMove");
    const Rect& roi_move = task_ptr->rect_move;

    set_roi(Rect(pt.x, pt.y, 0, 0).move(roi_move));
}
