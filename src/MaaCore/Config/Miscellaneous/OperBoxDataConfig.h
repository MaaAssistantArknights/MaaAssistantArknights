#pragma once
#include "Common/AsstBattleDef.h"
#include "Common/AsstMsg.h"
#include "Config/AbstractConfig.h"
#include "Vision/Oper/OperBoxImageAnalyzer.h"

#include <optional>
#include <vector>

namespace asst
{
class AbstractTask;

// 干员识别数据（OperBox）解析与辅助编队预检
class OperBoxDataConfig final : public MAA_NS::SingletonHolder<OperBoxDataConfig>, public AbstractConfig
{
public:
    virtual ~OperBoxDataConfig() override = default;

    void set_task(AbstractTask* task) { m_task_ptr = task; }

    void set_ignore_requirements(bool ignore_requirements) { m_ignore_requirements = ignore_requirements; }

    const std::vector<OperBoxInfo>& get_data() const noexcept { return m_data; }

    void clear();

    std::optional<battle::copilot::OperUsageGroups>
        precheck(const battle::copilot::OperUsageGroups& formation, bool use_support_unit);

protected:
    virtual bool parse(const json::value& data) override;

private:
    bool can_match(const battle::copilot::OperUsageGroup& group, const OperBoxInfo& info) const;

    std::vector<OperBoxInfo> m_data;
    AbstractTask* m_task_ptr = nullptr;
    bool m_ignore_requirements = false;
};

inline static auto& OperBoxData = OperBoxDataConfig::get_instance();
}
