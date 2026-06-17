#pragma once

#ifdef __ANDROID__

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Common/AsstTypes.h"
#include "MaaUtils/NoWarningCV.hpp" // DetBox 用到 cv::Point2f，需完整 cv 类型

namespace ncnn
{
class Net;
}

namespace asst
{

class OcrPackNcnn
{
public:
    using Result = TextRect;
    using ResultsVec = std::vector<Result>;

    OcrPackNcnn();
    ~OcrPackNcnn();

    // det/rec 的 .ncnn.param/.bin + rec 的 keys.txt
    bool load(
        const std::filesystem::path& det_param,
        const std::filesystem::path& det_bin,
        const std::filesystem::path& rec_param,
        const std::filesystem::path& rec_bin,
        const std::filesystem::path& keys_path);

    bool initialized() const { return m_loaded; }

    void set_cpu_threads(int num) { m_cpu_threads = num > 0 ? num : 1; }

    // without_det 时把整张 image 当作一行文本只跑 rec；否则 det -> 裁切 -> rec。
    // 返回的 rect 为 4 点框的轴对齐外接矩形（与 fastdeploy 版语义一致）。
    ResultsVec recognize(const cv::Mat& image, bool without_det);

private:
    // DBNet 检测出的一个候选框（已还原到原图坐标系）
    struct DetBox
    {
        cv::Point2f pts[4]; // 左上、右上、右下、左下
        float score = 0.f;
    };

    std::vector<DetBox> detect(const cv::Mat& image) const;
    // 透视摆正裁出文本行（竖排自动旋正）
    static cv::Mat crop_rotated(const cv::Mat& image, const DetBox& box);
    // 单行文本识别：返回 (utf-8 文本, 平均置信度)
    std::pair<std::string, float> recognize_line(const cv::Mat& line) const;

    std::unique_ptr<ncnn::Net> m_det;
    std::unique_ptr<ncnn::Net> m_rec;
    std::vector<std::string> m_charset; // [<blank>] + keys (+ space)，按 rec 输出维度对齐
    int m_cpu_threads = 2;
    bool m_loaded = false;
    bool m_rec_output_is_prob = false;
};
}

#endif // __ANDROID__
