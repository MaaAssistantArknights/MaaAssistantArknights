#include "InstProps.h"

#include "Assistant.h"

asst::InstProps::InstProps(asst::Assistant* inst) : m_inst(inst) {}

std::shared_ptr<asst::Controller> asst::InstProps::ctrler() const
{
    return m_inst ? m_inst->ctrler() : nullptr;
}
std::shared_ptr<asst::Status> asst::InstProps::status() const
{
    return m_inst ? m_inst->status() : nullptr;
}
bool asst::InstProps::need_exit() const
{
    return m_inst ? m_inst->need_exit() : false;
}
