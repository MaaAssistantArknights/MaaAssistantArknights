#pragma once
#include "Common/AsstBattleDef.h"
#include "Common/AsstMsg.h"
#include "Config/AbstractConfig.h"
#include "Vision/Oper/OperBoxImageAnalyzer.h"

#include <functional>
#include <optional>
#include <vector>

namespace asst
{
// 干员识别数据（OperBox）解析与辅助编队预检
class OperBoxDataConfig final : public MAA_NS::SingletonHolder<OperBoxDataConfig>, public AbstractConfig
{
public:
    using Callback = std::function<void(AsstMsg msg, const json::value& details)>;
    using NeedExit = std::function<bool()>;

public:
    virtual ~OperBoxDataConfig() override = default;

    void set_callback(Callback callback) { m_callback = std::move(callback); }

    void set_need_exit(NeedExit need_exit) { m_need_exit = std::move(need_exit); }

    const std::vector<OperBoxInfo>& get_data() const noexcept { return m_data; }

    void clear();

    std::optional<battle::copilot::OperUsageGroups>
        precheck(const battle::copilot::OperUsageGroups& formation, bool use_support_unit);

protected:
    virtual bool parse(const json::value& data) override;

private:
    std::vector<OperBoxInfo> m_data;
    Callback m_callback = [](AsstMsg, const json::value&) {
    };
    NeedExit m_need_exit = []() {
        return false;
    };
};

inline static auto& OperBoxData = OperBoxDataConfig::get_instance();
}
