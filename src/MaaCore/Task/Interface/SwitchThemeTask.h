#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class SwitchThemeTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "SwitchTheme";

    SwitchThemeTask(const AsstCallback& callback, Assistant* inst);
    virtual ~SwitchThemeTask() override = default;

    virtual bool set_params(const json::value& params) override;

private:
    virtual bool run() override;

    std::vector<std::string> m_candidates; // 候选主题名，多个时随机抽取
};
}
