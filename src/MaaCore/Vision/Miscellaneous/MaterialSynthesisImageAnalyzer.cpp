#include "MaterialSynthesisImageAnalyzer.h"

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"

namespace
{
constexpr std::string_view MaterialTask = "MiniGame@MaterialSynthesis@Material";
}

bool asst::MaterialSynthesisImageAnalyzer::analyze()
{
    m_result = { };

    const auto task_ptr = Task.get<MatchTaskInfo>(std::string(MaterialTask));

    Matcher matcher(m_image);
    matcher.set_task_info(task_ptr);

    for (const auto& item_id : ItemData.get_ordered_material_item_id()) {
        if (!ItemData.get_item_rarity(item_id)) {
            continue;
        }

        matcher.set_templ(item_id);
        const auto result = matcher.analyze();
        if (result && result->score > m_result.score) {
            m_result = *result;
        }
    }

    if (m_result.templ_name.empty()) {
        Log.warn("MaterialSynthesis | material template match failed");
        return false;
    }

    Log.info(
        "MaterialSynthesis | material template matched",
        m_result.templ_name,
        ItemData.get_item_name(m_result.templ_name),
        m_result.score);
    return true;
}
