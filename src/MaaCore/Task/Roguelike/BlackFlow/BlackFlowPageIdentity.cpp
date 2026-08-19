#include "BlackFlowSession.h"
#include "BlackFlowTaskPort.h"

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

    const bool final = texts.contains("险路尽头");
    const bool shop = texts.contains("前瞻性投资系统");
    const bool scrap_shop = texts.contains("机械师的园圃");
    const bool emergency_aid = texts.size() == 1 && texts.contains("刷新");
    const int classifications = static_cast<int>(final) + static_cast<int>(shop) + static_cast<int>(scrap_shop) +
                                static_cast<int>(emergency_aid);
    if (classifications > 1) {
        observation.classification_conflict = true;
    }
    else if (final) {
        observation.classified_type = NodeType::Final;
    }
    else if (shop) {
        observation.classified_type = NodeType::Shop;
    }
    else if (scrap_shop) {
        observation.classified_type = NodeType::ScrapShop;
    }
    else if (emergency_aid) {
        observation.classified_type = NodeType::Employ;
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
