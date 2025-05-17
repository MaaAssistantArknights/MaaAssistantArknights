#pragma once

#include "Common/AsstMsg.h"
#include "InstHelper.h"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include <chrono>

namespace asst
{

class Watchdog : private InstHelper
{
public:
    Watchdog(int matchIntervalSec, int freezeThreshold, const AsstCallback& callback, Assistant* inst);
    ~Watchdog();

    std::chrono::steady_clock::time_point next_wakeup_time() const;

    bool check(const cv::Mat& frame);

    void reset(int matchIntervalSec = -1, int freezeThreshold = -1);

private:
    const int defaultMatchIntervalSec = 60;
    const int defaultFreezeThreshold = 5;
    int m_matchIntervalSec;
    int m_freezeThreshold;
    AsstCallback m_callback;

    std::chrono::steady_clock::time_point m_lastMatchTime;
    int m_consecutiveMatches;
    cv::Mat m_bufferFrame;

    void callback(AsstMsg msg, const json::value& details);

    static bool is_similar(const cv::Mat& a, const cv::Mat& b, double threshold = 0.95);
};

} // namespace asst
