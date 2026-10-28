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

    void set_collect_material_shortfall(bool enabled) { m_collect_material_shortfall = enabled; }

    const auto& get_material_shortfall() const noexcept { return m_material_shortfall; }

    const auto& get_material_shortfall_context() const noexcept { return m_material_shortfall_context; }

protected:
    virtual bool _run() override;

private:
    enum class Result
    {
        Completed,
        InsufficientResources,
        OperatorUnavailable,
        Unsupported,
        NavigationFailed,
        Cancelled,
    };

    Result synthesize_material(
        int depth,
        std::unordered_set<std::string>& material_stack,
        int& operation_budget,
        InfrastProcessingTask& processing_task,
        bool& operator_selection_initialized);
    Result select_processing_operator(
        const std::string& material_id,
        int material_rarity,
        bool operator_missing,
        bool& operator_changed,
        InfrastProcessingTask& processing_task);

    bool run_task(const std::string& task_name, int retry_times = 3);
    bool detect_task(const std::string& task_name);
    bool return_to_workshop();
    std::optional<std::string> recognize_material();
    void report_status(std::string what, json::value details = json::object());
    void report_result(Result result);

    static std::string_view result_name(Result result);

    bool m_collect_material_shortfall = false;
    std::optional<std::pair<std::string, int>> m_material_shortfall;
    std::string m_material_shortfall_context;
};
} // namespace asst
