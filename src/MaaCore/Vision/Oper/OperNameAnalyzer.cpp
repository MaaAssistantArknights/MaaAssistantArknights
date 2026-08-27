#include "OperNameAnalyzer.h"

#include "MaaUtils/NoWarningCV.hpp"
#include "Vision/RegionOCRer.h"

asst::OperNameAnalyzer::ResultOpt asst::OperNameAnalyzer::analyze() const
{
    int kPadding = m_params.bin_expansion;

    cv::Mat gray, bin;
    cv::Mat ocr = make_roi(m_image, m_roi);

    if (ocr.empty()) {
        return std::nullopt;
    }

    cv::cvtColor(ocr, gray, cv::COLOR_BGR2GRAY);
    cv::inRange(gray, m_params.bin_threshold_lower, m_params.bin_threshold_upper, bin);

    int move = 0;
    bool isRight = (m_text_alignment == TextAlignment::Right);

    int start = isRight ? bin.cols - 1 : 0;
    int end = isRight ? -1 : bin.cols;
    int step = isRight ? -1 : 1;

    // 1. 裁剪首个包含文字的列（Left = 左边界，Right = 右边界），保留 padding
    for (int r = start; r != end; r += step) {
        if (cv::hasNonZero(bin.col(r))) {
            int cut = isRight ? (bin.cols - r - 1 - kPadding) : (r - kPadding);
            cut = std::clamp(cut, 0, bin.cols - 1);

            if (cut > 0) {
                bin = isRight ? bin.adjustROI(0, 0, 0, -cut) : bin.adjustROI(0, 0, -cut, 0);
                ocr = isRight ? ocr.adjustROI(0, 0, 0, -cut) : ocr.adjustROI(0, 0, -cut, 0);

                if (!isRight) {
                    move = cut;
                }
            }
            break;
        }
    }

    if (bin.empty() || bin.cols <= 0 || bin.rows <= 0) {
        return std::nullopt;
    }

    // 2. 底部裁剪（防止裁过头）
    if (m_bottom_line_height > 0 && m_bottom_line_height < bin.rows) {
        bin = bin.adjustROI(0, -m_bottom_line_height, 0, 0);
        ocr = ocr.adjustROI(0, -m_bottom_line_height, 0, 0);
    }

    if (bin.cols < m_width_threshold) {
        return std::nullopt;
    }

    // 3. 扫描连续空白块，裁剪另一侧
    start = isRight ? (bin.cols - m_width_threshold) : 0;
    end = isRight ? -1 : (bin.cols - m_width_threshold + 1);

    for (int r = start; r != end; r += step) {
        int rEnd = r + m_width_threshold;
        if (r < 0 || rEnd > bin.cols) {
            continue;
        }

        if (!cv::hasNonZero(bin.colRange(r, rEnd))) {
            if (isRight) {
                int cut = rEnd - kPadding;
                cut = std::clamp(cut, 0, ocr.cols - 1);
                move = cut;
                ocr = ocr.adjustROI(0, 0, -cut, 0);
            }
            else {
                int cut = std::clamp(r + kPadding, 0, ocr.cols);
                int rightTrim = ocr.cols - cut;
                rightTrim = std::clamp(rightTrim, 0, ocr.cols - 1);
                ocr = ocr.adjustROI(0, 0, 0, -rightTrim);
            }
            break;
        }
    }

    if (ocr.empty() || ocr.cols <= 0 || ocr.rows <= 0) {
        return std::nullopt;
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
