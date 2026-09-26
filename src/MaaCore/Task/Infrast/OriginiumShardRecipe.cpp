#include "OriginiumShardRecipe.h"

#include "Config/TaskData.h"
#include "Task/ProcessTask.h"
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

bool asst::run_originium_shard_recipe_task(const AbstractTask& task, OriginiumShardRecipe recipe)
{
    ProcessTask select_task(task, { std::string(originium_shard_recipe_task_name(recipe)) });
    if (!select_task.run()) {
        return false;
    }

    // ProcessTask 达到 maxTimes 后也可能结束为成功，必须额外确认已经离开配方选择流程。
    ProcessTask verify_task(task, { "VerifyMfgProductDetailsPage" });
    return verify_task.run();
}

bool asst::restore_mfg_product_details_page(const AbstractTask& task)
{
    constexpr int MaxReturnTimes = 3;

    auto is_product_details_page = [&]() {
        ProcessTask verify_task(task, { "VerifyMfgProductDetailsPage" });
        return verify_task.run();
    };

    if (is_product_details_page()) {
        return true;
    }

    // 选择配方失败时可能停在配方页、数量页或确认弹窗，逐层返回到产品详情页。
    for (int i = 0; i < MaxReturnTimes; ++i) {
        ProcessTask return_task(task, { "Return" });
        if (!return_task.run()) {
            return false;
        }
        if (is_product_details_page()) {
            return true;
        }
    }

    return false;
}
