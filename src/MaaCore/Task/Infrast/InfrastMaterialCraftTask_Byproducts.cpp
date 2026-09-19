#include "InfrastMaterialCraftTask.h"

#include <charconv>
#include <chrono>
#include <cmath>

#include "Config/Miscellaneous/ItemConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Utils/Logger.hpp"
#include "Utils/ProcessingByproductAccumulator.h"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

using namespace asst;

MaterialInventory InfrastMaterialCraftTask::read_processing_byproducts(const cv::Mat& toast_image, int batches) const
{
    MultiMatcher labels(toast_image);
    labels.set_task_info("MaterialCraft-ByproductToast");
    labels.set_roi({ 0, 0, toast_image.cols, toast_image.rows });
    const auto matches = labels.analyze();
    MaterialInventory result;
    if (!matches) {
        return result;
    }
    for (const auto& match : *matches) {
        RegionOCRer name(toast_image);
        name.set_task_info("MaterialCraft-ByproductName");
        auto roi = match.rect.move(Task.get("MaterialCraft-ByproductName")->rect_move);
        roi.width = std::min(roi.width, toast_image.cols - roi.x);
        name.set_roi(roi);
        const auto find_item_id = [](const auto& text) {
            std::string id_result;
            if (!text || !std::isfinite(text->score) || text->score < 0.9) {
                return id_result;
            }
            for (const auto& id : ItemData.get_ordered_material_item_id()) {
                if (ItemData.get_item_name(id) == text->text) {
                    if (!id_result.empty()) {
                        return std::string();
                    }
                    id_result = id;
                }
            }
            return id_result;
        };
        auto text = name.analyze();
        auto item_id = find_item_id(text);
        if (item_id.empty()) {
            // The cyan label has its strongest contrast in the blue channel.
            // Enlarge it for the uncommon characters in newer material names.
            const auto region = make_rect<cv::Rect>(roi) & cv::Rect(0, 0, toast_image.cols, toast_image.rows);
            if (region.empty()) {
                continue;
            }
            std::vector<cv::Mat> channels;
            cv::split(toast_image(region), channels);
            cv::Mat enlarged;
            cv::resize(channels[0], enlarged, cv::Size(), 3, 3, cv::INTER_CUBIC);
            cv::cvtColor(enlarged, enlarged, cv::COLOR_GRAY2BGR);
            RegionOCRer retry(enlarged);
            retry.set_task_info("MaterialCraft-ByproductName");
            retry.set_roi({ 0, 0, enlarged.cols, enlarged.rows });
            text = retry.analyze();
            item_id = find_item_id(text);
        }
        int count_vertical_offset = 0;
        if (item_id.empty() && text && text->text == "技巧概要" && std::isfinite(text->score) && text->score >= 0.9) {
            const auto suffix_task = Task.get("MaterialCraft-ByproductNameWrappedSuffix");
            RegionOCRer suffix(toast_image);
            suffix.set_task_info("MaterialCraft-ByproductNameWrappedSuffix");
            suffix.set_roi(match.rect.move(suffix_task->rect_move));
            if (const auto line = suffix.analyze(); line && std::isfinite(line->score) && line->score >= 0.9) {
                text->text += "·" + line->text;
                text->score = std::min(text->score, line->score);
                item_id = find_item_id(text);
                count_vertical_offset = suffix_task->special_params[0];
            }
        }
        if (item_id.empty()) {
            continue;
        }
        const auto count_task =
            batches < 10 ? "MaterialCraft-ByproductCountSingleDigit" : "MaterialCraft-ByproductCount";
        auto count_roi = match.rect.move(Task.get(count_task)->rect_move);
        count_roi.y += count_vertical_offset;
        const auto read_count = [&](const cv::Mat& source) {
            RegionOCRer count(source);
            count.set_task_info(count_task);
            count.set_roi(count_roi);
            return count.analyze();
        };
        auto number = read_count(toast_image);
        if (!number || !std::isfinite(number->score) || number->score < 0.95) {
            // White quantity text overlaps some colored item artwork (e.g. fuscous fiber).
            // Taking the darkest RGB channel suppresses colored highlights while preserving the digits.
            std::vector<cv::Mat> channels;
            cv::split(toast_image, channels);
            cv::Mat neutral;
            cv::min(channels[0], channels[1], neutral);
            cv::min(neutral, channels[2], neutral);
            cv::cvtColor(neutral, neutral, cv::COLOR_GRAY2BGR);
            number = read_count(neutral);
        }
        if (!number || !std::isfinite(number->score) || number->score < 0.95) {
            continue;
        }
        int quantity = 0;
        const auto& value = number->text;
        const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), quantity);
        if (error != std::errc() || end != value.data() + value.size() || quantity <= 0 || quantity > batches) {
            continue;
        }
        result[item_id] += quantity;
    }
    return result;
}

void InfrastMaterialCraftTask::capture_processing_byproducts(
    const cv::Mat& first_reward,
    int batches,
    const std::vector<cv::Mat>& pending_frames)
{
    using Clock = std::chrono::steady_clock;
    const auto config = Task.get("MaterialCraft-ByproductCapture");
    const auto& params = config->special_params;
    const auto minimum = std::chrono::milliseconds(params[0]);
    const auto quiet = std::chrono::milliseconds(params[1]);
    const auto max_frames = static_cast<size_t>(params[2]);
    const auto roi = make_rect<cv::Rect>(Task.get("MaterialCraft-ByproductRegion")->roi);
    const auto began = Clock::now();
    const auto deadline = began + std::chrono::seconds(4 + 2 * std::clamp(batches, 1, 24));
    auto last_toast = began;
    std::vector<cv::Mat> frames = pending_frames;
    auto image = first_reward;
    // Capture first. OCR can be slower than the toast animation, so run it only on the saved frames.
    do {
        if (image.empty() || roi.x + roi.width > image.cols || roi.y + roi.height > image.rows) {
            break;
        }
        auto toast = image(roi);
        Matcher label(toast);
        label.set_task_info("MaterialCraft-ByproductToast");
        label.set_roi({ 0, 0, toast.cols, toast.rows });
        const bool visible = label.analyze().has_value();
        const auto now = Clock::now();
        if (visible) {
            last_toast = now;
            frames.emplace_back(toast.clone());
        }
        if (now >= deadline || frames.size() >= max_frames ||
            (!visible && now - began >= minimum && now - last_toast >= quiet) || !craft_sleep(config->post_delay)) {
            break;
        }
        image = ctrler()->get_image();
    } while (!need_exit());

    ProcessingByproductAccumulator accumulator(batches);
    for (const auto& frame : frames) {
        // Even on cancellation, account for already confirmed output using buffered evidence.
        accumulator.observe(read_processing_byproducts(frame, batches));
    }
    m_processing_byproducts = accumulator.confirmed();
    Log.info("MaterialCraft | byproduct frames", frames.size(), "confirmed types", m_processing_byproducts.size());
    // A screenshot gap or a missed toast is indistinguishable from a failed random roll.
    // Known byproducts are applied, while the inventory remains marked as potentially incomplete.
    m_inventory_complete = false;
}
