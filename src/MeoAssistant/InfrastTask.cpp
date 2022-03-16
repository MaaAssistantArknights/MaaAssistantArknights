#include "InfrastTask.h"

#include "Logger.hpp"

#include "ProcessTask.h"
#include "InfrastInfoTask.h"
#include "InfrastMfgTask.h"
#include "InfrastTradeTask.h"
#include "InfrastPowerTask.h"
#include "InfrastControlTask.h"
#include "InfrastReceptionTask.h"
#include "InfrastOfficeTask.h"
#include "InfrastDormTask.h"
#include "DronesForShamareTaskPlugin.h"
#include "ReplenishOriginiumShardTaskPlugin.h"

asst::InfrastTask::InfrastTask(AsstCallback callback, void* callback_arg)
    : PackageTask(callback, callback_arg, TaskType),
    m_infrast_begin_task_ptr(std::make_shared<ProcessTask>(callback, callback_arg, TaskType)),
    m_info_task_ptr(std::make_shared<InfrastInfoTask>(callback, callback_arg, TaskType)),
    m_mfg_task_ptr(std::make_shared<InfrastMfgTask>(callback, callback_arg, TaskType)),
    m_trade_task_ptr(std::make_shared<InfrastTradeTask>(callback, callback_arg, TaskType)),
    m_power_task_ptr(std::make_shared<InfrastPowerTask>(callback, callback_arg, TaskType)),
    m_control_task_ptr(std::make_shared<InfrastControlTask>(callback, callback_arg, TaskType)),
    m_reception_task_ptr(std::make_shared<InfrastReceptionTask>(callback, callback_arg, TaskType)),
    m_office_task_ptr(std::make_shared<InfrastOfficeTask>(callback, callback_arg, TaskType)),
    m_dorm_task_ptr(std::make_shared<InfrastDormTask>(callback, callback_arg, TaskType))
{
    m_infrast_begin_task_ptr->set_tasks({ "InfrastBegin" });
    m_trade_task_ptr->regiseter_plugin<DronesForShamareTaskPlugin>();
    m_replenish_task_ptr = m_mfg_task_ptr->regiseter_plugin<ReplenishOriginiumShardTaskPlugin>();

    m_subtasks.emplace_back(m_infrast_begin_task_ptr);
}

bool asst::InfrastTask::set_params(const json::value& params)
{
    if (!m_runned) {
        if (!params.contains("facility") || !params.at("facility").is_array()) {
            return false;
        }

        auto append_infrast_begin = [&]() {
            m_subtasks.emplace_back(m_infrast_begin_task_ptr);
        };

        m_subtasks.clear();
        append_infrast_begin();

        for (const auto& facility_json : params.at("facility").as_array()) {
            if (!facility_json.is_string()) {
                m_subtasks.clear();
                append_infrast_begin();
                return false;
            }

            std::string facility = facility_json.as_string();
            if (facility == "Dorm") {
                m_subtasks.emplace_back(m_dorm_task_ptr);
            }
            else if (facility == "Mfg") {
                m_subtasks.emplace_back(m_mfg_task_ptr);
            }
            else if (facility == "Trade") {
                m_subtasks.emplace_back(m_trade_task_ptr);
            }
            else if (facility == "Power") {
                m_subtasks.emplace_back(m_power_task_ptr);
            }
            else if (facility == "Office") {
                m_subtasks.emplace_back(m_office_task_ptr);
            }
            else if (facility == "Reception") {
                m_subtasks.emplace_back(m_reception_task_ptr);
            }
            else if (facility == "Control") {
                m_subtasks.emplace_back(m_control_task_ptr);
            }
            else {
                Log.error(__FUNCTION__, "| Unknown facility", facility);
                m_subtasks.clear();
                append_infrast_begin();
                return false;
            }
            append_infrast_begin();
        }
    }

    std::string drones = params.get("drones", "_NotUse");
    m_mfg_task_ptr->set_uses_of_drone(drones);
    m_trade_task_ptr->set_uses_of_drone(drones);

    double threshold = params.get("threshold", 0.3);
    m_info_task_ptr->set_mood_threshold(threshold);
    m_mfg_task_ptr->set_mood_threshold(threshold);
    m_trade_task_ptr->set_mood_threshold(threshold);
    m_power_task_ptr->set_mood_threshold(threshold);
    m_control_task_ptr->set_mood_threshold(threshold);
    m_reception_task_ptr->set_mood_threshold(threshold);
    m_office_task_ptr->set_mood_threshold(threshold);
    m_dorm_task_ptr->set_mood_threshold(threshold);

    bool replenish = params.get("replenish", false);
    m_replenish_task_ptr->set_enable(replenish);

    return true;
}
