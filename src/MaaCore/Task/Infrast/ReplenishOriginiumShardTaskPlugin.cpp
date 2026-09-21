#include "ReplenishOriginiumShardTaskPlugin.h"

#include "Task/ProcessTask.h"
#include "Utils/StringMisc.hpp"

bool asst::ReplenishOriginiumShardTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskExtraInfo || details.get("subtask", std::string()) != "InfrastMfgTask") {
        return false;
    }

    if (details.at("what").as_string() == "ProductOfFacility" &&
        details.at("details").at("product").as_string() == "OriginStone") {
        return true;
    }
    else {
        return false;
    }
}

std::optional<asst::ReplenishOriginiumShardTaskPlugin::MaterialCount>
asst::ReplenishOriginiumShardTaskPlugin::parse_material_count(std::string_view text, int expected_required) noexcept
{
    const auto separator = text.find('/');
    if (separator == std::string_view::npos || text.find('/', separator + 1) != std::string_view::npos) {
        return std::nullopt;
    }

    int current = 0;
    int required = 0;
    if (!utils::chars_to_number<int, true>(text.substr(0, separator), current) ||
        !utils::chars_to_number<int, true>(text.substr(separator + 1), required) || current < 0 ||
        required != expected_required) {
        return std::nullopt;
    }

    return MaterialCount { .current = current, .required = required };
}

bool asst::ReplenishOriginiumShardTaskPlugin::_run()
{
    ProcessTask task(*this, { "ReplenishToMax" });
    return task.run();
}
