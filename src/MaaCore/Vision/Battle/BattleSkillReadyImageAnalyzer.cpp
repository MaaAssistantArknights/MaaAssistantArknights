#include "BattleSkillReadyImageAnalyzer.h"

#include "Utils/NoWarningCV.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <onnxruntime/core/session/onnxruntime_cxx_api.h>

#include "Config/OnnxSessions.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"

bool asst::BattleSkillReadyImageAnalyzer::analyze()
{
    cv::Mat image = m_image(make_rect<cv::Rect>(m_roi));
    std::vector<float> input = image_to_tensor(image);

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
    constexpr int64_t batch_size = 1;
    std::array<int64_t, 4> input_shape { batch_size, image.channels(), image.cols, image.rows };

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input.data(), input.size(),
                                                              input_shape.data(), input_shape.size());

    constexpr size_t classification_size = 2;
    std::array<float, classification_size> results;
    std::array<int64_t, 2> output_shape { batch_size, classification_size };
    Ort::Value output_tensor = Ort::Value::CreateTensor<float>(memory_info, results.data(), results.size(),
                                                               output_shape.data(), output_shape.size());

    auto& session = OnnxSessions::get_instance().get("skill_ready_cls");
    // 这俩是hardcode在模型里的
    constexpr const char* input_names[] = { "input" };   // session.GetInputName()
    constexpr const char* output_names[] = { "output" }; // session.GetOutputName()

    Ort::RunOptions run_options;
    session.Run(run_options, input_names, &input_tensor, 1, output_names, &output_tensor, 1);
    Log.info(__FUNCTION__, "raw result: ", results);

    softmax(results);
    Log.info(__FUNCTION__, "after softmax: ", results);
    bool ready = results[1] > results[0];

#ifdef ASST_DEBUG
    if (ready) {
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(m_roi), cv::Scalar(0, 165, 255), 2);
        cv::putText(m_image_draw, std::to_string(results[1]), cv::Point(m_roi.x, m_roi.y - 10), 1, 1.2,
                    cv::Scalar(0, 165, 255), 2);
    }
#endif

    return ready;
}

void asst::BattleSkillReadyImageAnalyzer::set_base_point(const Point& pt)
{
    auto task_ptr = Task.get<MatchTaskInfo>("BattleSkillReady");
    const Rect& skill_roi_move = task_ptr->rect_move;

    set_roi(Rect(pt.x, pt.y, 0, 0).move(skill_roi_move));
}
