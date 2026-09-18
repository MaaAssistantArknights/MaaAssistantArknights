#pragma once
#include "Common/AsstBattleDef.h"
#include "Common/AsstMsg.h"
#include "Config/AbstractConfig.h"
#include "Vision/Oper/OperBoxImageAnalyzer.h"

#include <optional>
#include <unordered_map>
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
    bool can_match(const battle::OperUsage& usage, const OperBoxInfo& info) const;
    std::vector<std::vector<size_t>>
        get_adjacency(const battle::copilot::OperUsageGroups& formation, const std::vector<OperBoxInfo>& data) const;
    std::vector<std::vector<size_t>> get_adjacency(
        const battle::copilot::OperUsageGroups& formation,
        const std::vector<OperBoxInfo>& data,
        size_t start_pos,
        size_t end_pos,
        std::string fake_oper_id) const;

    std::vector<OperBoxInfo> m_data;
    std::unordered_map<std::string, size_t> m_oper_id_to_index;
    AbstractTask* m_task_ptr = nullptr;
    bool m_ignore_requirements = false;
};

inline static auto& OperBoxData = OperBoxDataConfig::get_instance();
}
