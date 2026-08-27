#include "OperDetailPage.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Task/ProcessTask.h"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

asst::OperDetailPage::OperDetailPage(
    const AsstCallback& callback,
    Assistant* inst,
    std::string_view task_chain) :
    InstHelper(inst),
    m_callback(callback),
    m_task_chain(task_chain)
{
}

std::optional<int> asst::OperDetailPage::analyze_elite(const cv::Mat& image)
{
    const static auto& elite_task = Task.get("OperDetailElite");
    Matcher matcher(image);
    matcher.set_task_info(elite_task);
    if (!matcher.analyze()) {
        LogError << __FUNCTION__ << "analyze elite failed";
        return std::nullopt;
    }

    int elite = 0;
    auto length = matcher.get_result().templ_name.size();
    auto elite_str = matcher.get_result().templ_name.substr(length - 1);

    if (elite_str.empty() || !utils::chars_to_number(elite_str, elite)) {
        LogError << __FUNCTION__ << "convert elite failed, template:" << matcher.get_result().templ_name
                 << ", str:" << elite_str;
        return std::nullopt;
    }
    return elite;
}

std::optional<int> asst::OperDetailPage::analyze_level(const cv::Mat& image)
{
    const static auto& level_task = Task.get("OperDetailLevel");
    RegionOCRer ocrer(image);
    ocrer.set_task_info(level_task);
    if (!ocrer.analyze()) {
        LogError << __FUNCTION__ << "ocr oper level failed, str:" << ocrer.get_result().text;
        return std::nullopt;
    }
    int level = 0;
    if (!utils::chars_to_number(ocrer.get_result().text, level)) {
        LogError << __FUNCTION__ << "convert oper level failed, str:" << ocrer.get_result().text;
        return std::nullopt;
    }
    return level;
}

std::optional<int> asst::OperDetailPage::analyze_skill_level(const cv::Mat& image)
{
    const static auto& level_base = Task.get("OperDetailSkillLevel-Base");
    RegionOCRer ocrer(image);
    ocrer.set_task_info(level_base);
    if (!ocrer.analyze()) {
        LogError << __FUNCTION__ << "ocr skill level base failed, str:" << ocrer.get_result().text;
        return std::nullopt;
    }

    int base_level = 0;
    if (!utils::chars_to_number(ocrer.get_result().text, base_level)) {
        LogError << __FUNCTION__ << "convert skill level base failed, str:" << ocrer.get_result().text;
        return std::nullopt;
    }

    return base_level;
}

