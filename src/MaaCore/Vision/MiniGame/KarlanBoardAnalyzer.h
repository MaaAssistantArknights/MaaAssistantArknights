#pragma once

#include "MaaUtils/NoWarningCV.hpp"
#include "Task/MiniGame/KarlanPlanner.h"

namespace asst::karlan
{
class BoardAnalyzer
{
public:
    explicit BoardAnalyzer(const cv::Mat& image);
    std::optional<int> stage() const;
    bool rules_visible() const;
    bool selected(int slot) const;
    std::vector<int> hand() const;
    std::optional<Board> read(const Rules& rule, std::optional<int> expected, int last_yield) const;
    static int slot_x(int slot);

private:
    cv::Mat m_image;
    cv::Mat m_gray;
    cv::Mat m_color;
};
}
