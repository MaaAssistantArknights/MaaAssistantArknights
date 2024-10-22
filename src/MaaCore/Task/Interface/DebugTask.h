#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class DebugTask : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "Debug";

    DebugTask(const AsstCallback& callback, Assistant* inst);
    virtual ~DebugTask() override = default;

    virtual bool run() override;

private:
    void test_drops();
    void test_skill_ready();
    void test_battle_image();
    void test_match_template();
};
}
