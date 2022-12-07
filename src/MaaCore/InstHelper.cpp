#include "InstHelper.h"

#include "Assistant.h"

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