#include "InstHelper.h"

#include <sstream>

#include "Assistant.h"
#include "Utils/Logger.hpp"

asst::InstHelper::InstHelper(asst::Assistant* inst) :
    m_inst(inst)
{
}

std::shared_ptr<asst::Controller> asst::InstHelper::ctrler() const
{
    return m_inst ? m_inst->ctrler() : nullptr;
}

std::shared_ptr<asst::Status> asst::InstHelper::status() const
{
    return m_inst ? m_inst->status() : nullptr;
}

bool asst::InstHelper::need_exit() const
{
    return m_inst != nullptr && m_inst->need_exit();
}

bool asst::InstHelper::sleep(unsigned millisecond) const
{
    if (need_exit()) {
        return false;
    }
    if (millisecond == 0) {
        std::this_thread::yield();
        return true;
    }
    Log.trace("ready to sleep", millisecond);
    auto millisecond_ms = std::chrono::milliseconds(millisecond);
    auto interval = std::chrono::milliseconds(std::min(millisecond, 5000U));

    for (auto sleep_time = interval; sleep_time <= millisecond_ms && !need_exit(); sleep_time += interval) {
        std::this_thread::sleep_for(interval);
    }
    if (!need_exit()) {
        std::this_thread::sleep_for(millisecond_ms % interval);
    }
    Log.trace("end of sleep", millisecond);

    return !need_exit();
}

asst::Assistant* asst::InstHelper::inst() noexcept
{
    return m_inst;
}

std::string asst::InstHelper::inst_string() const
{
    std::stringstream ss;
    ss << m_inst;
    return ss.str();
}
