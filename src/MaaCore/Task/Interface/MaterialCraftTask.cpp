#include "MaterialCraftTask.h"

#include "Task/Infrast/InfrastMaterialCraftTask.h"

asst::MaterialCraftTask::MaterialCraftTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_material_craft_task_ptr(std::make_shared<InfrastMaterialCraftTask>(callback, inst, TaskType))
{
    m_subtasks.emplace_back(m_material_craft_task_ptr);
}

bool asst::MaterialCraftTask::set_params(const json::value& params)
{
    return m_material_craft_task_ptr->set_params(params);
}
