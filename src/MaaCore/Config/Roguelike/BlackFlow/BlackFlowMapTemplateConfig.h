#pragma once

#include "Config/AbstractConfig.h"
#include "Task/Roguelike/BlackFlow/BlackFlowMapTopology.h"
#include "Task/Roguelike/BlackFlow/BlackFlowModel.h"

namespace asst::blackflow
{
struct MapTemplate
{
    MapTopology id;
    int floor = 0;
    int rows = 0;
    int columns = 0;
    // 模板出生点用于加载时的图结构校验，不参与运行时匹配。
    GridPosition start;
    NodeType terminal_type = NodeType::Unknown;
    std::vector<GridPosition> terminals;
    std::vector<std::pair<GridPosition, GridPosition>> edges;
};
} // namespace asst::blackflow

namespace asst
{
class BlackFlowMapTemplateConfig final :
    public MAA_NS::SingletonHolder<BlackFlowMapTemplateConfig>,
    public AbstractConfig
{
public:
    [[nodiscard]] const std::vector<blackflow::MapTemplate>& templates() const noexcept { return m_templates; }

protected:
    bool parse(const json::value& value) override;

private:
    std::vector<blackflow::MapTemplate> m_templates;
};

inline auto& BlackFlowMapTemplates = BlackFlowMapTemplateConfig::get_instance();
} // namespace asst
