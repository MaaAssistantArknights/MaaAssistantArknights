#include "InstHelper.h"

#include "Assistant.h"
#include "Utils/Logger.hpp"

asst::InstHelper::InstHelper(asst::Assistant* inst) : m_inst(inst) {}

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
    return m_inst ? m_inst->need_exit() : false;
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
    auto start = std::chrono::steady_clock::now();
    Log.trace("ready to sleep", millisecond);
    auto millisecond_ms = std::chrono::milliseconds(millisecond);
    auto interval = millisecond_ms / 5;

    while (!need_exit()) {
        std::this_thread::sleep_for(interval);
        if (std::chrono::steady_clock::now() - start > millisecond_ms) {
            break;
        }
    }
    Log.trace("end of sleep", millisecond);

    return !need_exit();
}
