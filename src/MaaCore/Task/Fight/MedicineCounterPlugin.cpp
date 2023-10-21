#include "MedicineCounterPlugin.h"

#include <ranges>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "InstHelper.h"
#include "Task/Fight/DrGrandetTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

bool asst::MedicineCounterPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    return details.at("details").at("task").as_string().ends_with("UseMedicine");
}

bool asst::MedicineCounterPlugin::_run()
{
    LogTraceFunction;

    if (m_using_count >= m_max_count && !m_use_expiring) {
        return true;
    }

    // 现在葛朗台插件在前，暂时不需要延迟
    // sleep(Task.get("MedicineReduceIcon")->pre_delay);
    auto image = ctrler()->get_image();

    // 预计最大使用药品情况不会超过2种，不考虑滑动
    // 10+10+N
    auto initialCount = initial_count(image);
    if (!initialCount) [[unlikely]] {
        return false;
    }

    auto refresh_medicine_count = [&]() {
        image = ctrler()->get_image();
        auto count = initial_count(image);
        if (!count) [[unlikely]] {
            return false;
        }
        initialCount = count;
        return true;
    };
    if (m_using_count < m_max_count && initialCount->using_count + m_using_count > m_max_count) {
        reduce_excess(*initialCount);
        if (!refresh_medicine_count()) {
            return false;
        }
    }
    else if (m_using_count >= m_max_count && m_use_expiring) {
        bool changed = false;
        for (auto& [use, inventory, rect, is_expiring] : initialCount->medicines | std::ranges::views::reverse) {
            while (use > 0 && is_expiring != ExpiringStatus::Expiring) {
                ctrler()->click(rect);
                sleep(Config.get_options().task_delay);
                use--;
                changed = true;
            }
        }

        if (changed && !refresh_medicine_count()) {
            return false;
        }
    }
    m_using_count += initialCount->using_count;

    // 博朗台：如果溢出则等待
    if (m_dr_grandet) {
        auto sanity_target = get_target_of_sanity(image);
        auto sanity_max = get_maximun_of_sanity(image);
        if (!sanity_target || !sanity_max) [[unlikely]] {
            return false;
        }

        if (*sanity_target >= *sanity_max) [[unlikely]] {
            auto waitTime = DrGrandetTaskPlugin::analyze_time_left(image);
            sleep(waitTime);
        }
    }

    ProcessTask(*this, { "MedicineConfirm" }).run();

    auto info = basic_info_with_what("UseMedicine");
    info["details"]["is_expiring"] = m_using_count > m_max_count;
    info["details"]["count"] = initialCount->using_count;
    callback(AsstMsg::SubTaskExtraInfo, info);
    return true;
}

std::optional<asst::MedicineCounterPlugin::InitialMedicineResult> asst::MedicineCounterPlugin::initial_count(
    cv::Mat image)
{
    int use = 0;
    MultiMatcher multiMatcher(image);
    multiMatcher.set_task_info("MedicineReduceIcon");
    if (!multiMatcher.analyze()) {
        Log.error(__FUNCTION__, "medicine reduce icon analyze failed");
        return std::nullopt;
    }

    std::vector<Medicine> medicines;
    for (const auto& result : multiMatcher.get_result()) {
        auto using_rect = result.rect.move(Task.get("MedicineUsingCount")->rect_move);
        auto inventory_rect = result.rect.move(Task.get("MedicineInventory")->rect_move);
        auto expiring_rect = result.rect.move(Task.get("MedicineExpiringTime")->rect_move);

        RegionOCRer using_ocr(image);
        using_ocr.set_task_info("MedicineUsingCount");
        using_ocr.set_bin_threshold(100, 255);
        using_ocr.set_roi(using_rect);
        if (!using_ocr.analyze()) {
            Log.error(__FUNCTION__, "medicine using count analyze failed");
            return std::nullopt;
        }

        RegionOCRer inventry_ocr(image);
        inventry_ocr.set_task_info("MedicineUsingCount");
        inventry_ocr.set_bin_threshold(100, 255);
        inventry_ocr.set_roi(inventory_rect);
        if (!inventry_ocr.analyze()) {
            Log.error(__FUNCTION__, "medicine using count analyze failed");
            return std::nullopt;
        }

        auto is_expiring = ExpiringStatus::UnSure;
        if (m_using_count >= m_max_count) {
            RegionOCRer expiring_ocr(image);
            expiring_ocr.set_task_info("MedicineExpiringTime");
            expiring_ocr.set_roi(expiring_rect);
            if (expiring_ocr.analyze()) {
                is_expiring = ExpiringStatus::Expiring;
            }
            else {
                is_expiring = ExpiringStatus::NotExpiring;
            }
        }

        int using_count = 0, inventory_count = 0;
        if (!utils::chars_to_number(using_ocr.get_result().text, using_count) ||
            !utils::chars_to_number(inventry_ocr.get_result().text, inventory_count)) {
            return std::nullopt;
        }
        use += using_count;
        medicines.emplace_back(Medicine { .use = using_count,
                                          .inventry = inventory_count,
                                          .reduce_button_position = result.rect,
                                          .is_expiring = is_expiring });
    }
    return InitialMedicineResult { .using_count = use, .medicines = medicines };
}

void asst::MedicineCounterPlugin::reduce_excess(InitialMedicineResult& using_medicine)
{
    auto reduce = m_using_count + using_medicine.using_count - m_max_count;
    for (auto& [use, inventory, rect, is_expiring] : using_medicine.medicines | std::ranges::views::reverse) {
        while (use > 0 && reduce > 0) {
            ctrler()->click(rect);
            sleep(Config.get_options().task_delay);
            use--;
            reduce--;
            using_medicine.using_count--;
        }
    }
}

std::optional<int> asst::MedicineCounterPlugin::get_target_of_sanity(const cv::Mat& image)
{
    RegionOCRer ocr(image);
    ocr.set_bin_threshold(100, 255);
    ocr.set_task_info("UsingMedicine-Target");
    if (!ocr.analyze()) [[unlikely]] {
        Log.error(__FUNCTION__, "unable to ocr");
        return std::nullopt;
    }
    int num = 0;
    if (!utils::chars_to_number(ocr.get_result().text, num)) {
        Log.error(__FUNCTION__, "unable to change [", ocr.get_result().text, "] into int");
        return false;
    }
    return num;
}

std::optional<int> asst::MedicineCounterPlugin::get_maximun_of_sanity(const cv::Mat& image)
{
    RegionOCRer ocr(image);
    ocr.set_bin_threshold(100, 255);
    ocr.set_task_info("UsingMedicine-SanityMax");
    if (!ocr.analyze()) [[unlikely]] {
        Log.error(__FUNCTION__, "unable to ocr");
        return std::nullopt;
    }
    int num = 0;
    if (!utils::chars_to_number(ocr.get_result().text, num)) {
        Log.error(__FUNCTION__, "unable to change [", ocr.get_result().text, "] into int");
        return false;
    }
    return num;
}
