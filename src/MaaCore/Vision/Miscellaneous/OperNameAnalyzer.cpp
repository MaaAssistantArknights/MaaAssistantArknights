#include "OperNameAnalyzer.h"

#include "Utils/NoWarningCV.h"
#include "Vision/RegionOCRer.h"

asst::OperNameAnalyzer::ResultOpt asst::OperNameAnalyzer::analyze() const
{
    cv::Mat gray, bin;
    cv::Mat ocr = make_roi(m_image, m_roi);
    cv::cvtColor(ocr, gray, cv::COLOR_BGR2GRAY);
    cv::inRange(gray, m_params.bin_threshold_lower, m_params.bin_threshold_upper, bin);
    for (int r = bin.cols - 1; r >= 0; --r) {
        if (cv::hasNonZero(bin.col(r))) { // 右边界向左收缩
            bin = bin.adjustROI(0, 0, 0, -(bin.cols - r - 1));
            break;
        }
    }

    bin = bin.adjustROI(0, -m_bottom_line_height, 0, 0); // 底部收缩排除白线
    int move = 0;
    for (int r = bin.cols - m_width_threshold - 1; r >= 0; --r) {
        if (!cv::hasNonZero(bin.colRange(r, r + m_width_threshold))) { // 左边界排除无文字区域
            move = r + m_width_threshold - 3;
            ocr = ocr.adjustROI(0, 0, -move, 0);
            break;
        }
    }

    RegionOCRer region(ocr);
    region.set_params(m_params);
    region.set_use_raw(true);
    if (region.analyze()) {
        m_result = region.get_result();
        m_result.rect.x += move; // 修正偏移
        m_result.rect = m_roi.move(m_result.rect);
        return m_result;
    }

    return std::nullopt;
}
