#pragma once
#include "Task/InterfaceTask.h"

namespace asst
{
class InfrastPresetWriteTask;

class InfrastPresetImportTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "InfrastPresetImport";
    InfrastPresetImportTask(const AsstCallback& callback, Assistant* inst);
    bool set_params(const json::value& params) override;

private:
    std::shared_ptr<InfrastPresetWriteTask> m_writer;
};
}
