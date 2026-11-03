#include "VisionToolkit.h"
#include "Vision/Battle/BattlefieldMatcher.h"

namespace asst::experiment // 不知道写namespace合适不合适，反正AI这么建议的，先放这了
{
cv::Mat VisionToolkit::normalize_battlefield_frame(const cv::Mat& frame)
{
    if (frame.empty() || frame.rows <= 0 || frame.cols <= 0) {
        return cv::Mat::zeros(720, 1280, CV_8UC3);
    }
    int raw_w = frame.cols;
    int raw_h = frame.rows;
    double target_ratio = 1280.0 / 720.0;

    double current_ratio = static_cast<double>(raw_w) / raw_h;

    cv::Mat stitched = cv::Mat::zeros(720, 1280, frame.type());

    if (current_ratio > target_ratio) {
        // 对于宽屏锁定高度为 720
        double scale_h = 720.0 / raw_h;
        cv::Mat resized;
        cv::resize(frame, resized, cv::Size(), scale_h, scale_h, cv::INTER_AREA);

        int new_w = resized.cols;

        // 计算需要挖掉的总宽度，以及每个切口需要挖掉的宽度
        int redundant_w = new_w - 1280;
        int cut_w = redundant_w / 2;

        int q1 = new_w / 4;     // 左 1/4 处
        int q3 = new_w * 3 / 4; // 右 3/4 处

        // 从 0 到 1/4
        int left_w = q1 - cut_w / 2;
        cv::Mat left_part = resized(cv::Rect(0, 0, left_w, 720)).clone();

        // 从 1/4 到 3/4 (挖掉左右切口宽度的一半)
        int center_start = q1 + cut_w / 2;
        int center_w = (q3 - cut_w / 2) - center_start;
        cv::Mat center_part = resized(cv::Rect(center_start, 0, center_w, 720)).clone();

        // 从 3/4 到画面最右侧
        int right_start = q3 + cut_w / 2;
        int right_w = new_w - right_start;
        cv::Mat right_part =
            resized(cv::Rect(right_start, 0, right_w, 720) & cv::Rect(0, 0, resized.cols, 720)).clone();

        // 贴到 1280x720 的画布上
        int current_x = 0;
        left_part.copyTo(stitched(cv::Rect(current_x, 0, left_w, 720)));

        current_x += left_w;
        center_part.copyTo(stitched(cv::Rect(current_x, 0, center_w, 720)));

        current_x += center_w;
        int remaining_space = stitched.cols - current_x;
        int final_w = std::min(right_part.cols, remaining_space);
        right_part(cv::Rect(0, 0, final_w, 720)).copyTo(stitched(cv::Rect(current_x, 0, final_w, 720)));
    }
    else {
        // 窄屏锁定宽度 1280 保留顶部和底部 UI
        double scale_w = 1280.0 / raw_w;
        cv::Mat resized;
        cv::resize(frame, resized, cv::Size(), scale_w, scale_w, cv::INTER_AREA);

        int new_h = resized.rows;

        cv::Mat top_360 = resized(cv::Rect(0, 0, 1280, 360));
        cv::Mat bottom_360 = resized(cv::Rect(0, new_h - 360, 1280, 360));

        top_360.copyTo(stitched(cv::Rect(0, 0, 1280, 360)));
        bottom_360.copyTo(stitched(cv::Rect(0, 360, 1280, 360)));
    }

    return stitched;
}

BattlefieldMatcher::ResultOpt
    VisionToolkit::analyze_battlefield_ui(const cv::Mat& frame, std::optional<int> total_kills)
{
    cv::Mat standard_frame = normalize_battlefield_frame(frame);

    BattlefieldMatcher ui_analyzer(standard_frame);

    if (total_kills.has_value()) {
        ui_analyzer.set_object_of_interest({ .flag = true, .kills = true, .speed_button = true });
        ui_analyzer.set_total_kills_prompt(total_kills.value());
    }
    else {
        ui_analyzer.set_object_of_interest({ .flag = true, .kills = false, .speed_button = true });
    }

    return ui_analyzer.analyze();
}
}
