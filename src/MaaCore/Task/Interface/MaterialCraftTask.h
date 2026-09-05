#pragma once

#include "Task/InterfaceTask.h"

namespace asst
{
class InfrastMaterialCraftTask;

class MaterialCraftTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "MaterialCraft";

    MaterialCraftTask(const AsstCallback& callback, Assistant* inst);
    virtual ~MaterialCraftTask() override = default;

    virtual bool set_params(const json::value& params) override;

private:
    std::shared_ptr<InfrastMaterialCraftTask> m_material_craft_task_ptr = nullptr;
};
}
