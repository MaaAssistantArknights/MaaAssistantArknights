#include "CopilotTask.h"

#include "Resource.h"
#include "ProcessTask.h"
#include "BattleProcessTask.h"
#include "BattleFormationTask.h"

asst::CopilotTask::CopilotTask(AsstCallback callback, void* callback_arg)
	: PackageTask(callback, callback_arg, TaskType),
	m_battle_task_ptr(std::make_shared<BattleProcessTask>(callback, callback_arg, TaskType)),
	m_formation_task_ptr(std::make_shared<BattleFormationTask>(callback, callback_arg, TaskType))
{
	m_subtasks.emplace_back(m_formation_task_ptr);

	auto start_task_ptr = std::make_shared<ProcessTask>(callback, callback_arg, TaskType);
	start_task_ptr->set_tasks({ "BattleStartNormal", "BattleStartRaid", "BattleStartExercise" });
	m_subtasks.emplace_back(start_task_ptr);

	m_subtasks.emplace_back(m_battle_task_ptr);
}

bool asst::CopilotTask::set_params(const json::value& params)
{
	if (m_runned) {
		return false;
	}

	if (!params.contains("stage_name") || !params.at("stage_name").is_string()) {
		return false;
	}
	std::string stage_name = params.at("stage_name").as_string();

	m_battle_task_ptr->set_stage_name(stage_name);
	m_formation_task_ptr->set_stage_name(stage_name);

	bool with_formation = params.get("formation", false);
	m_formation_task_ptr->set_enable(with_formation);

	std::string filename = params.get("filename", std::string());
	// 文件名为空时，不加载资源，直接返回 true
	return filename.empty() || Resrc.copilot().load(filename);
}