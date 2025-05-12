#include "Watchdog.h"
#include <meojson/json.hpp>

namespace asst
{

Watchdog::Watchdog(int matchIntervalSec, int freezeThreshold, const AsstCallback& callback, Assistant* inst) :
    InstHelper(inst),
    m_matchIntervalSec(matchIntervalSec > 0 ? matchIntervalSec : defaultMatchIntervalSec),
    m_freezeThreshold(freezeThreshold > 0 ? freezeThreshold : defaultFreezeThreshold),
    m_callback(callback),
    m_lastMatchTime(std::chrono::steady_clock::now()),
    m_consecutiveMatches(0)
{
}

Watchdog::~Watchdog()
{
}

std::chrono::steady_clock::time_point Watchdog::next_wakeup_time() const
{
    return m_lastMatchTime + std::chrono::seconds(m_matchIntervalSec);
}

bool Watchdog::check(const cv::Mat& frame)
{
    if (!m_bufferFrame.empty()) {
        if (is_similar(frame, m_bufferFrame)) {
            m_consecutiveMatches++;
        }
        else {
            m_consecutiveMatches = 0;
        }
        m_lastMatchTime = std::chrono::steady_clock::now();
    }
    Log.debug("consecutive matches:", m_consecutiveMatches);
    m_bufferFrame = frame.clone();
    if (m_consecutiveMatches >= m_freezeThreshold) {
        m_consecutiveMatches = 0;
        return false;
    }
    return true;
}

void Watchdog::reset(int matchIntervalSec, int freezeThreshold)
{
    m_matchIntervalSec = (matchIntervalSec > 0) ? matchIntervalSec : m_matchIntervalSec;
    m_freezeThreshold = (freezeThreshold > 0) ? freezeThreshold : m_freezeThreshold;
    m_bufferFrame.release();
    m_consecutiveMatches = 0;
    m_lastMatchTime = std::chrono::steady_clock::now();
}

bool Watchdog::is_similar(const cv::Mat& a, const cv::Mat& b, double threshold)
{
    if (a.empty() || b.empty() || a.size() != b.size() || a.type() != b.type()) {
        return false;
    }

    cv::Mat result;
    cv::matchTemplate(a, b, result, cv::TM_CCOEFF_NORMED);
    return result.at<float>(0, 0) >= threshold;
}

void Watchdog::callback(AsstMsg msg, const json::value& details)
{
    if (m_callback) {
        m_callback(msg, details, m_inst);
    }
}
} // namespace asst
