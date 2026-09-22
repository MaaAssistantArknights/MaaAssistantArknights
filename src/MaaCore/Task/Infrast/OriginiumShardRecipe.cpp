#include "OriginiumShardRecipe.h"

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/RegionOCRer.h"

namespace asst::originium_shard_recipe_impl
{
struct MaterialCount
{
    int current = 0;
    int required = 0;
};

std::optional<MaterialCount> parse_material_count(std::string_view text, int expected_required) noexcept
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

std::optional<MaterialCount> ocr_material_count(const cv::Mat& image, std::string_view task_name, int expected_required)
{
    const auto ocr_task = Task.get<OcrTaskInfo>(std::string(task_name));
    if (ocr_task == nullptr) {
        LogError << __FUNCTION__ << "| missing OCR task:" << task_name;
        return std::nullopt;
    }

    RegionOCRer analyzer(image);
    analyzer.set_task_info(ocr_task);
    if (!analyzer.analyze()) {
        LogWarn << __FUNCTION__ << "| OCR failed:" << task_name;
        return std::nullopt;
    }

    return parse_material_count(analyzer.get_result().text, expected_required);
}
}

std::optional<asst::OriginiumShardRecipe> asst::detect_originium_shard_recipe(const cv::Mat& image, bool allow_device)
{
    if (!allow_device) {
        return OriginiumShardRecipe::OriginiumOre;
    }

    const auto originium_ore = originium_shard_recipe_impl::ocr_material_count(image, "InfrastMfgOriginiumOreCount", 2);
    const auto device = originium_shard_recipe_impl::ocr_material_count(image, "InfrastMfgDeviceCount", 1);
    if (!originium_ore || !device) {
        return std::nullopt;
    }

    if (originium_ore->current >= originium_ore->required) {
        return OriginiumShardRecipe::OriginiumOre;
    }
    if (device->current >= device->required) {
        return OriginiumShardRecipe::Device;
    }
    return std::nullopt;
}

std::string_view asst::originium_shard_recipe_task_name(OriginiumShardRecipe recipe) noexcept
{
    switch (recipe) {
    case OriginiumShardRecipe::OriginiumOre:
        return "ChooseOriginiumShardFromOriginiumOre";
    case OriginiumShardRecipe::Device:
        return "ChooseOriginiumShardFromDevice";
    }
    return {};
}
