#include "MaterialSynthesisImageAnalyzer.h"

#include "MaaUtils/NoWarningCV.hpp"

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Config/TemplResource.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"

namespace
{
constexpr std::string_view MaterialTask = "MiniGame@MaterialSynthesis@Material";
constexpr double MaterialTemplateScale = 1.2;
}

bool asst::MaterialSynthesisImageAnalyzer::analyze()
{
    m_result = { };

    const auto task_ptr = Task.get<MatchTaskInfo>(std::string(MaterialTask));

    Matcher matcher(m_image);
    matcher.set_task_info(task_ptr);

    for (const auto& item_id : ItemData.get_ordered_non_chip_formula_item_id()) {
        const cv::Mat& item_template = TemplResource::get_instance().get_templ(item_id);
        cv::Mat scaled_template;
        cv::resize(
            item_template,
            scaled_template,
            cv::Size(),
            MaterialTemplateScale,
            MaterialTemplateScale,
            cv::INTER_LINEAR);

        matcher.set_templ(std::move(scaled_template));
        const auto result = matcher.analyze();
        if (result && result->score > m_result.score) {
            m_result = *result;
            m_result.templ_name = item_id;
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
