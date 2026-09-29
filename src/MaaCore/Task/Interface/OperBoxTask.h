#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class OperBoxTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "OperBox";

    OperBoxTask(const AsstCallback& callback, Assistant* inst);
    virtual ~OperBoxTask() override = default;

    virtual bool set_params(const json::value& params) override;
    virtual bool run() override;

private:
    void build_subtasks();
    bool m_paradox_filter = false;
    bool m_subtasks_built = false;
    bool m_next_only = false;
    std::vector<std::string> m_candidates;
};
}
