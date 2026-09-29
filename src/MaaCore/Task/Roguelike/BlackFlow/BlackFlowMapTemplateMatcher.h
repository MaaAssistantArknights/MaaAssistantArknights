#pragma once

#include <span>

#include "BlackFlowModel.h"
#include "Config/Roguelike/BlackFlow/BlackFlowMapTemplateConfig.h"

namespace asst::blackflow
{
[[nodiscard]] const MapTemplate*
    match_map_template(const MapObservationBatch& observation, std::span<const MapTemplate> templates);
void supplement_map_exits(MapObservationBatch& observation, std::span<const GridPosition> exits);
} // namespace asst::blackflow
