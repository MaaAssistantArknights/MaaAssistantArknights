#include "BlackFlowSession.h"
#include "BlackFlowTaskPort.h"

#include <array>
#include <iterator>
#include <set>
#include <utility>

namespace asst::blackflow
{
EnteredPageObservation classify_entered_page_texts(std::vector<std::string> matched_texts)
{
    std::set<std::string> texts(
        std::make_move_iterator(matched_texts.begin()),
        std::make_move_iterator(matched_texts.end()));
    EnteredPageObservation observation;
    observation.matched_texts.assign(texts.begin(), texts.end());

    static constexpr std::array identities {
        std::pair { "险路尽头", NodeType::Final },   std::pair { "三重身", NodeType::Evacuate },
        std::pair { "溯源", NodeType::Portal },      std::pair { "坎诺特", NodeType::Shop },
        std::pair { "机械师", NodeType::ScrapShop }, std::pair { "佩德洛", NodeType::Employ },
    };
    for (const auto& [text, type] : identities) {
        if (!texts.contains(text)) {
            continue;
        }
        if (observation.classified_type.has_value() && observation.classified_type != type) {
            observation.classified_type.reset();
            observation.classification_conflict = true;
            break;
        }
        observation.classified_type = type;
    }
    return observation;
}

PageIdentityResolution resolve_page_identity(
    NodeType map_type,
    std::string map_name,
    const MovePreview* preview,
    const EnteredPageObservation& entered_page)
{
    PageIdentityResolution result { map_type, std::move(map_name) };
    const bool map_identity_unresolved =
        map_type == NodeType::Unknown || map_type == NodeType::HideInvisible || map_type == NodeType::HideBattle;
    if (!map_identity_unresolved || entered_page.classification_conflict) {
        return result;
    }
    if (entered_page.classified_type.has_value()) {
        result.type = *entered_page.classified_type;
    }
    else if (preview != nullptr && preview->displayed_type != NodeType::Unknown) {
        result.type = preview->displayed_type;
    }
    if (preview != nullptr && !preview->displayed_name.empty()) {
        result.name = preview->displayed_name;
    }
    return result;
}
} // namespace asst::blackflow
