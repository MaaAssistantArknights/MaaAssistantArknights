#pragma once

#include "Task/AbstractTaskPlugin.h"

#include <optional>
#include <string>
#include <unordered_set>

namespace asst
{
class InfrastProcessingTask;

class MaterialSynthesisTaskPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~MaterialSynthesisTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

protected:
    virtual bool _run() override;

private:
    enum class Result
    {
        Completed,
        InsufficientResources,
        Unsupported,
        NavigationFailed,
        Cancelled,
    };

    Result synthesize_material(
        int depth,
        std::unordered_set<std::string>& material_stack,
        int& operation_budget,
        InfrastProcessingTask& processing_task);
    Result select_processing_operator(
        const std::string& material_id,
        int material_level,
        bool operator_missing,
        bool& operator_changed,
        InfrastProcessingTask& processing_task);

    bool run_task(const std::string& task_name, int retry_times = 3);
    bool detect_task(const std::string& task_name);
    bool return_to_workshop();
    std::optional<int> read_number(const std::string& task_name);
    std::optional<std::string> read_text(const std::string& task_name);
    std::string find_item_id(const std::string& name) const;

    static int item_level(const std::string& item_id);
    static std::string_view result_name(Result result);
};
} // namespace asst
