#include "AbstractTaskPlugin.h"

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

bool asst::AbstractTaskPlugin::operator<(const asst::AbstractTaskPlugin& rhs) const
{
    return priority() < rhs.priority();
}
