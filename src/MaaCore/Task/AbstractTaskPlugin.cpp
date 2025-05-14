#include "AbstractTaskPlugin.h"

#include "Controller/Controller.h"
#include "ProcessTask.h"

int asst::AbstractTaskPlugin::priority() const
{
    return m_priority;
}

bool asst::AbstractTaskPlugin::block() const
{
    return m_block;
}

void asst::AbstractTaskPlugin::set_priority(int priority)
{
    m_priority = priority;
}

void asst::AbstractTaskPlugin::set_block(bool block)
{
    m_block = block;
}

void asst::AbstractTaskPlugin::set_task_ptr(asst::AbstractTask* ptr)
{
    m_task_ptr = ptr;
}

std::strong_ordering asst::AbstractTaskPlugin::operator<=>(const asst::AbstractTaskPlugin& rhs) const
{
    return priority() <=> rhs.priority();
}

bool asst::AbstractTaskPlugin::operator==(const AbstractTaskPlugin& rhs) const
{
    return priority() == rhs.priority();
}

cv::Mat asst::AbstractTaskPlugin::get_process_image() const
{
    if (auto ptr = dynamic_cast<ProcessTask*>(m_task_ptr)) {
        auto image = ptr->get_last_hit()->image;
        if (!image.empty()) {
            return image;
        }
    }
    return ctrler()->get_image();
}

template <typename T>
requires std::derived_from<T, asst::AnalyzerResult>
std::shared_ptr<T> asst::AbstractTaskPlugin::get_hit_detail() const
{
    if (auto ptr = dynamic_cast<ProcessTask*>(m_task_ptr)) {
        if (auto detail = std::dynamic_pointer_cast<T>(ptr->get_last_hit()->reco_detail)) {
            return detail;
        }
    }
    Log.error(__FUNCTION__, "| Unable to get hit detail of type:", typeid(T).name());
    return nullptr;
}
