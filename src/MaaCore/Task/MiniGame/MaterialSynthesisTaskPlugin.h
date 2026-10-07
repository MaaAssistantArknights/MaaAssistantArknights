#pragma once

#include "Common/AsstItemDef.h"
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
    enum class Result
    {
        Completed,
        MissingMaterial,
        OperatorUnavailable,
        Unsupported,
        RecognitionFailed,
        NavigationFailed,
        FormulaLocked,
        OperationLimit,
        Cancelled,
    };

    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~MaterialSynthesisTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

    Result get_result() const noexcept { return m_result; }

    const std::optional<MissingMaterial>& get_missing_material() const noexcept { return m_missing_material; }

    const std::string& get_missing_context() const noexcept { return m_missing_context; }

    int get_completed_operations() const noexcept { return m_completed_operations; }

    void set_check_processing_coins(bool enable) noexcept { m_check_processing_coins = enable; }

protected:
    virtual bool _run() override;

private:
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
    std::optional<int> read_number(const std::string& task_name, const cv::Mat& image = {});
    std::optional<int> read_exact_number(const std::string& task_name, const cv::Mat& image);
    std::optional<std::string> recognize_material(const cv::Mat& image = {});
    Result check_processing_coins(const std::string& material_id, int depth, int selected_count);
    void report_status(std::string what, json::value details = json::object());
    void report_result(Result result);

    static std::string_view result_name(Result result);

    Result m_result = Result::Unsupported;
    std::optional<MissingMaterial> m_missing_material;
    std::string m_missing_context;
    int m_completed_operations = 0;
    bool m_check_processing_coins = false;
};
} // namespace asst
