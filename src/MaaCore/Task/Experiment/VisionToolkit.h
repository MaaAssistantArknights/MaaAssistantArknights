#pragma once

#include "Vision/Battle/BattlefieldMatcher.h"
#include <memory>
#include <opencv2/opencv.hpp>

namespace asst::experiment
{

class VisionToolkit
{
public:
    /**
     * @brief 将任意分辨率/屏幕比例的画面,硬派适配至标准的 720p (1280x720), 此方法适用于组件的识别
     * @param frame 输入的原始设备画面
     * @return cv::Mat 适配后的 720p 标准画面
     */
    static cv::Mat normalize_battlefield_frame(const cv::Mat& frame);

    /**
     * @brief 自动化战场 UI 分析流程（识别 flag, kills(传入total_kills), speed_button 等）
     * @param frame 输入的原始设备画面
     * @param total_kills[可选] 传入的当前总击杀提示数
     * @return 自动推导 BattlefieldMatcher::analyze() 的返回类型（通常为 std::optional 或特定结果结构体）
     */
    static BattlefieldMatcher::ResultOpt
        analyze_battlefield_ui(const cv::Mat& frame, std::optional<int> total_kills = std::nullopt);
};

} // namespace asst::experiment
