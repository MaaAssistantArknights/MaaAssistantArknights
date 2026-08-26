#include "MaterialSynthesisImageAnalyzer.h"

#include <algorithm>

#include "MaaUtils/NoWarningCV.hpp"

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Config/TemplResource.h"
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
    const cv::Mat material_image = make_roi(m_image, task_ptr->roi);
    const cv::Scalar material_color = cv::mean(material_image(center_rect(material_image)));
    const double max_color_difference = task_ptr->special_params.at(0);

    Matcher matcher(m_image);
    matcher.set_task_info(task_ptr);

    for (const auto& item_id : ItemData.get_ordered_material_item_id()) {
        if (!ItemData.get_item_rarity(item_id)) {
            continue;
        }

        const cv::Mat& item_template = TemplResource::get_instance().get_templ(item_id);
        const cv::Scalar template_color = cv::mean(item_template(center_rect(item_template)));
        if (color_difference(material_color, template_color) > max_color_difference) {
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

double asst::MaterialSynthesisImageAnalyzer::color_difference(const cv::Scalar& lhs, const cv::Scalar& rhs)
{
    const double blue = lhs[0] - rhs[0];
    const double green = lhs[1] - rhs[1];
    const double red = lhs[2] - rhs[2];
    return blue * blue + green * green + red * red;
}

cv::Rect asst::MaterialSynthesisImageAnalyzer::center_rect(const cv::Mat& image, int width, int height)
{
    width = std::min(width, image.cols);
    height = std::min(height, image.rows);
    return { (image.cols - width) / 2, (image.rows - height) / 2, width, height };
}
