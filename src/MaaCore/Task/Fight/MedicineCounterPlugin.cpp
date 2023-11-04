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
    auto initial_count = init_count(image);
    if (!initial_count) [[unlikely]] {
        return false;
    }

    auto refresh_medicine_count = [&]() {
        image = ctrler()->get_image();
        auto count = init_count(image);
        if (!count) [[unlikely]] {
            return false;
        }
        initial_count = count;
        return true;
    };
    if (m_using_count < m_max_count && initial_count->using_count + m_using_count > m_max_count) {
        reduce_excess(*initial_count);
        if (!refresh_medicine_count()) {
            return false;
        }
    }
    else if (m_using_count >= m_max_count && m_use_expiring) {
        bool changed = false;
        for (const auto& [use, inventory, rect, is_expiring] :
             initial_count->medicines | asst::ranges::views::reverse) {
            if (use > 0 && is_expiring != ExpiringStatus::Expiring) {
                ctrler()->click(rect);
                sleep(Config.get_options().task_delay);
                changed = true;
            }
        }

        if (changed && !refresh_medicine_count()) {
            return false;
        }
    }
    m_using_count += initial_count->using_count;

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
    info["details"]["count"] = initial_count->using_count;
    callback(AsstMsg::SubTaskExtraInfo, info);
    return true;
}

std::optional<asst::MedicineCounterPlugin::InitialMedicineResult> asst::MedicineCounterPlugin::init_count(cv::Mat image)
{
    int use = 0;
    MultiMatcher multiMatcher(image);
    multiMatcher.set_task_info("MedicineReduceIcon");
    if (!multiMatcher.analyze()) {
        Log.error(__FUNCTION__, "medicine reduce icon analyze failed");
        return std::nullopt;
    }

    std::vector<Medicine> medicines;
    auto using_count_task = Task.get("UsingMedicineCount");
    auto inventory_task = Task.get("MedicineInventory");
    auto expiring_task = Task.get("MedicineExpiringTime");

    for (const auto& result : multiMatcher.get_result()) {
        auto using_rect = result.rect.move(using_count_task->rect_move);
        auto inventory_rect = result.rect.move(inventory_task->rect_move);
        auto expiring_rect = result.rect.move(expiring_task->rect_move);

        RegionOCRer using_count_ocr(image);
        using_count_ocr.set_task_info(using_count_task);
        using_count_ocr.set_bin_threshold(using_count_task->special_params[0], using_count_task->special_params[1]);
        using_count_ocr.set_roi(using_rect);
        if (!using_count_ocr.analyze()) {
            Log.error(__FUNCTION__, "medicine using count analyze failed");
            return std::nullopt;
        }

        RegionOCRer inventory_ocr(image);
        inventory_ocr.set_task_info(inventory_task);
        inventory_ocr.set_bin_threshold(inventory_task->special_params[0], inventory_task->special_params[1]);
        inventory_ocr.set_roi(inventory_rect);
        if (!inventory_ocr.analyze()) {
            Log.error(__FUNCTION__, "medicine using count analyze failed");
            return std::nullopt;
        }

        auto is_expiring = ExpiringStatus::UnSure;
        if (m_using_count >= m_max_count) {
            RegionOCRer expiring_ocr(image);
            expiring_ocr.set_task_info(expiring_task);
            expiring_ocr.set_roi(expiring_rect);
            if (expiring_ocr.analyze()) {
                is_expiring = ExpiringStatus::Expiring;
            }
            else {
                is_expiring = ExpiringStatus::NotExpiring;
            }
        }

        int using_count = 0, inventory_count = 0;
        if (!utils::chars_to_number(using_count_ocr.get_result().text, using_count) ||
            !utils::chars_to_number(inventory_ocr.get_result().text, inventory_count)) {
            Log.error(__FUNCTION__, "unable to convert ocr result to int, use:", using_count_ocr.get_result().text,
                      ", inventory:", inventory_ocr.get_result().text);
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

void asst::MedicineCounterPlugin::reduce_excess(const InitialMedicineResult& using_medicine)
{
    auto reduce = m_using_count + using_medicine.using_count - m_max_count;
    for (const auto& [use, inventory, rect, is_expiring] : using_medicine.medicines | std::ranges::views::reverse) {
        ctrler()->click(rect);
        sleep(Config.get_options().task_delay);
        reduce -= use;

        auto add_rect = rect.move(Task.get("UsingMedicineCount")->rect_move);
        while (reduce < 0) {
            ctrler()->click(add_rect);
            sleep(Config.get_options().task_delay);
            reduce++;
        }
        if (reduce == 0) {
            break;
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
