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

void asst::AbstractTaskPlugin::set_plugin_exit_flag(bool* exit_flag) noexcept
{
    AbstractTask::set_exit_flag(exit_flag);
}
