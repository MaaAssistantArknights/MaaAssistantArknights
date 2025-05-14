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
        auto image = ptr->get_result_image();
        if (!image.empty()) {
            return image;
        }
    }
    return ctrler()->get_image();
}
