#include "MedicineCounterTaskPlugin.h"

#include <ranges>

#include "Config/TaskData.h"
#include "Config/GeneralConfig.h"
#include "Controller/Controller.h"
#include "InstHelper.h"
#include "Task/Fight/DrGrandetTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

bool asst::MedicineCounterTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    return details.get("details", "task", "").ends_with("UseMedicine");
}

bool asst::MedicineCounterTaskPlugin::_run()
{
    LogTraceFunction;

    if (m_used_count >= m_max_count && !m_use_expiring) {
        LogTrace << __FUNCTION__ << "Needn't to use medicines"
                 << ",used:" << m_used_count << ",max:" << m_max_count
                 << "use_expiring:" << m_use_expiring;
        return true;
    }

    // 现在葛朗台插件在前，暂时不需要延迟
    // sleep(Task.get("MedicineReduceIcon")->pre_delay);
    auto image = ctrler()->get_image();

    // 预计最大使用药品情况不会超过2种，不考虑滑动
    // 10+10+N
    auto using_medicine = init_count(image);
    if (!using_medicine) [[unlikely]] {
        return false;
    }
    LogTrace << __FUNCTION__ << "Using medicines init finished,"
             << " using:" << using_medicine->using_count << "used:" << m_used_count;

    // 移除超量使用的理智药后，再次获取理智药数量
    // 如果移除后没有使用任何理智药，则单独返回数据；进入插件时应当有使用至少一瓶药
    auto refresh_medicine_count = [&]() {
        image = ctrler()->get_image();
        auto count = init_count(image);
        if (!count) {
            // 判断是不是还在使用理智药的页面
            Matcher matcher(image);
            matcher.set_task_info("UseMedicine");
            if (!matcher.analyze()) [[unlikely]] {
                Log.error(__FUNCTION__, "unable to analyze UseMedicine");
                return false;
            }
            using_medicine = MedicineResult { .using_count = 0, .medicines = {} };
        }
        else {
            using_medicine = count;
        }
        return true;
    };

    if (m_used_count < m_max_count && using_medicine->using_count + m_used_count > m_max_count) {
        reduce_excess(*using_medicine);
        if (!refresh_medicine_count()) {
            return false;
        }
    }
    else if (m_used_count >= m_max_count && m_use_expiring) {
        bool changed = false;
        for (const auto& [use, inventory, rect, is_expiring] :
             using_medicine->medicines | views::reverse) {
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

    if (using_medicine->using_count == 0) {
        return true;
    }

    m_used_count += using_medicine->using_count;

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

    ProcessTask(*this, { "MedicineConfirm" }).set_retry_times(5).run();

    auto info = basic_info_with_what("UseMedicine");
    info["details"]["is_expiring"] = m_used_count > m_max_count;
    info["details"]["count"] = using_medicine->using_count;
    callback(AsstMsg::SubTaskExtraInfo, info);
    return true;
}

std::optional<asst::MedicineCounterTaskPlugin::MedicineResult>
    asst::MedicineCounterTaskPlugin::init_count(cv::Mat image)
{
    int use = 0;
    MultiMatcher multi_matcher(image);
    multi_matcher.set_task_info("MedicineReduceIcon");
    if (!multi_matcher.analyze()) {
        Log.error(__FUNCTION__, "medicine reduce icon analyze failed");
        return std::nullopt;
    }

    std::vector<Medicine> medicines;
    static const auto& using_count_task = Task.get("UsingMedicineCount");
    static const auto& inventory_task = Task.get("MedicineInventory");
    static const auto& expiring_task = Task.get("MedicineExpiringTime");

    auto match_result = multi_matcher.get_result();
    sort_by_horizontal_(match_result); // 排序以保证结果为从左到右
    for (const auto& result : match_result) {
        auto using_rect = result.rect.move(using_count_task->rect_move);
        auto inventory_rect = result.rect.move(inventory_task->rect_move);
        auto expiring_rect = result.rect.move(expiring_task->rect_move);

        RegionOCRer using_count_ocr(image);
        using_count_ocr.set_task_info(using_count_task);
        using_count_ocr.set_bin_threshold(
            using_count_task->special_params[0],
            using_count_task->special_params[1]);
        using_count_ocr.set_roi(using_rect);
        if (!using_count_ocr.analyze()) {
            Log.error(__FUNCTION__, "medicine using count analyze failed");
            return std::nullopt;
        }

        RegionOCRer inventory_ocr(image);
        inventory_ocr.set_task_info(inventory_task);
        inventory_ocr.set_bin_threshold(
            inventory_task->special_params[0],
            inventory_task->special_params[1]);
        inventory_ocr.set_roi(inventory_rect);
        if (!inventory_ocr.analyze()) {
            Log.error(__FUNCTION__, "medicine using count analyze failed");
            return std::nullopt;
        }

        auto is_expiring = ExpiringStatus::UnSure;
        if (m_used_count >= m_max_count) {
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
        if (!utils::chars_to_number(using_count_ocr.get_result().text, using_count)
            || !utils::chars_to_number(inventory_ocr.get_result().text, inventory_count)) {
            LogError << __FUNCTION__ << "unable to convert ocr result to int,"
                     << "use:" << using_count_ocr.get_result().text << ","
                     << "inventory:" << inventory_ocr.get_result().text;
            return std::nullopt;
        }
        use += using_count;
        medicines.emplace_back(Medicine { .use = using_count,
                                          .inventry = inventory_count,
                                          .reduce_button_position = result.rect,
                                          .is_expiring = is_expiring });
        LogTrace << __FUNCTION__ << "medicine using count:" << using_count << ","
                 << "inventory count:" << inventory_count << ","
                 << "is expiring:" << static_cast<int>(is_expiring);
    }
    return MedicineResult { .using_count = use, .medicines = medicines };
}

void asst::MedicineCounterTaskPlugin::reduce_excess(const MedicineResult& using_medicine)
{
    auto reduce = m_used_count + using_medicine.using_count - m_max_count;
    for (const auto& [use, inventory, rect, is_expiring] :
         using_medicine.medicines | views::reverse) {
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

std::optional<int> asst::MedicineCounterTaskPlugin::get_target_of_sanity(const cv::Mat& image)
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

std::optional<int> asst::MedicineCounterTaskPlugin::get_maximun_of_sanity(const cv::Mat& image)
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
