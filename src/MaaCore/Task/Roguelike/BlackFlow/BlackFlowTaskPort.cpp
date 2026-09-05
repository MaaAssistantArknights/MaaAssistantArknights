#include "BlackFlowTaskPort.h"

#include "BlackFlowInventoryCleanup.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "BlackFlowMovementRecognition.h"

#include "Vision/Roguelike/BlackFlow/BlackFlowFloor.h"

#include "Config/Roguelike/BlackFlow/BlackFlowNodeExecutionConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/AbstractTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/OCRer.h"

namespace asst::blackflow
{
namespace
{
constexpr std::string_view CurrentActionPointsTask = "BlackFlow@Roguelike@CurrentActionPoints";
constexpr std::string_view MovePreviewWaitTask = "BlackFlow@Roguelike@MovePreviewWait";
constexpr std::string_view MovePreviewEnterTask = "BlackFlow@Roguelike@MovePreviewEnter";
constexpr std::string_view MovePreviewCannotEnterTask = "BlackFlow@Roguelike@MovePreviewCannotEnter";
constexpr std::string_view MovePreviewCostTask = "BlackFlow@Roguelike@MovePreviewCost";
constexpr std::string_view MovePreviewDisplayedNameTask = "BlackFlow@Roguelike@MovePreviewDisplayedName";
constexpr std::string_view MovePreviewConfirmTask = "BlackFlow@Roguelike@MovePreviewConfirm";
constexpr std::string_view MovePreviewConfirmCompletedTask = "BlackFlow@Roguelike@MovePreviewConfirmCompleted";
constexpr std::string_view EnteredPageClassificationTask = "BlackFlow@Roguelike@EnteredPageClassification";
constexpr std::string_view EnteredPageClassificationEncounterPrepareTask =
    "BlackFlow@Roguelike@EnteredPageClassificationEncounterPrepare";
constexpr std::string_view StageEncounterOcrTask = "BlackFlow@Roguelike@StageEncounterOcr";

void set_error(std::string* error, std::string message)
{
    if (error != nullptr) {
        *error = std::move(message);
    }
}

std::optional<int> recognize_integer(const cv::Mat& image, std::string_view task_name)
{
    const auto task = Task.get<OcrTaskInfo>(task_name);
    if (task == nullptr) {
        return std::nullopt;
    }
    OCRer analyzer(image);
    analyzer.set_task_info(task);
    const auto results = analyzer.analyze();
    if (!results.has_value() || results->size() != 1) {
        return std::nullopt;
    }
    int value = 0;
    if (!utils::chars_to_number(results->front().text, value)) {
        return std::nullopt;
    }
    return value;
}

std::optional<std::string>
    recognize_text(const cv::Mat& image, std::string_view task_name, const std::vector<std::string>& required = {})
{
    const auto task = Task.get<OcrTaskInfo>(task_name);
    if (task == nullptr) {
        return std::nullopt;
    }
    OCRer analyzer(image);
    analyzer.set_task_info(task);
    if (!required.empty()) {
        analyzer.set_required(required);
    }
    const auto results = analyzer.analyze();
    if (!results.has_value()) {
        return std::nullopt;
    }
    const std::vector<std::string>& whitelist = required.empty() ? task->text : required;
    for (const auto& result : *results) {
        if (std::ranges::find(whitelist, result.text) != whitelist.end()) {
            return result.text;
        }
    }
    return std::nullopt;
}

Rect shrink_to_center(const Rect& rect, double ratio)
{
    const int width = std::max(1, static_cast<int>(rect.width * ratio));
    const int height = std::max(1, static_cast<int>(rect.height * ratio));
    return Rect { rect.x + (rect.width - width) / 2, rect.y + (rect.height - height) / 2, width, height };
}

} // namespace

class BlackFlowTaskPort::ProcessTaskContext final : public AbstractTask, public BlackFlowInventoryContext
{
public:
    ProcessTaskContext(const AsstCallback& callback, Assistant* inst, std::string_view task_chain) :
        AbstractTask(callback, inst, task_chain)
    {
        set_retry_times(0);
    }

    bool execute(std::vector<std::string> tasks, std::string* error) override
    {
        m_tasks = std::move(tasks);
        m_last_task.clear();
        m_last_image.reset();
        const bool succeeded = AbstractTask::run();
        if (!succeeded) {
            set_error(error, "ProcessTask did not reach a successful terminal task");
        }
        return succeeded;
    }

    [[nodiscard]] const std::string& last_task() const noexcept override { return m_last_task; }

    [[nodiscard]] std::shared_ptr<cv::Mat> last_image() const noexcept { return m_last_image; }

    void report_cleanup_status(std::string_view status, std::string detail) override
    {
        auto info = basic_info_with_what("BlackFlowInventoryCleanup");
        json::object status_details { { "status", std::string(status) } };
        if (!detail.empty()) {
            status_details["name"] = std::move(detail);
        }
        info["details"] = std::move(status_details);
        callback(AsstMsg::SubTaskExtraInfo, info);
    }

    [[nodiscard]] cv::Mat capture() const override { return ctrler()->get_image(); }

    bool click(const Rect& rect) const override { return ctrler()->click(rect); }

    bool interrupted() const override { return need_exit(); }

    void wait(unsigned milliseconds) const override { sleep(milliseconds); }

    bool precise_swipe_supported() const override
    {
        return ControlFeat::support(ctrler()->support_features(), ControlFeat::PRECISE_SWIPE);
    }

    // 起点、时长、缓动仍取自任务配置，只有终点由调用方按当前画面计算。
    bool swipe_by(std::string_view task_name, int distance, std::string* error) override
    {
        const auto task = Task.get(std::string(task_name));
        if (task == nullptr) {
            set_error(error, "swipe task is missing: " + std::string(task_name));
            return false;
        }
        const Rect& start = task->specific_rect;
        const Rect end { start.x + distance, start.y, start.width, start.height };
        const auto& params = task->special_params;
        const bool swiped = ctrler()->swipe(
            start,
            end,
            params.empty() ? 0 : params.at(0),
            params.size() < 2 ? false : params.at(1) != 0,
            params.size() < 3 ? 1 : params.at(2),
            params.size() < 4 ? 1 : params.at(3));
        if (!swiped) {
            set_error(error, "swipe failed: " + std::string(task_name));
            return false;
        }
        sleep(task->post_delay);
        return true;
    }

protected:
    bool _run() override
    {
        ProcessTask process(*this, m_tasks);
        const bool succeeded = process.run();
        m_last_task = process.get_last_task_name();
        if (const auto& hit = process.get_last_hit(); hit != nullptr && hit->image != nullptr) {
            m_last_image = hit->image;
        }
        return succeeded;
    }

private:
    std::vector<std::string> m_tasks;
    std::string m_last_task;
    std::shared_ptr<cv::Mat> m_last_image;
};

BlackFlowTaskPort::BlackFlowTaskPort(
    const AsstCallback& callback,
    Assistant* inst,
    std::string_view task_chain,
    std::shared_ptr<IBlackFlowMapObservationSource> map_source) :
    m_task_context(std::make_unique<ProcessTaskContext>(callback, inst, task_chain)),
    m_inventory_cleanup(std::make_unique<BlackFlowInventoryCleanup>()),
    m_map_source(std::move(map_source))
{
}

BlackFlowTaskPort::~BlackFlowTaskPort() = default;

bool BlackFlowTaskPort::refresh(
    const BlackFlowObservationRequest& request,
    BlackFlowPerceptionSnapshot& snapshot,
    std::string* error)
{
    try {
        if (m_map_source == nullptr || m_task_context == nullptr) {
            set_error(error, "BlackFlow map observation source is not attached");
            return false;
        }
        if (!perception::floor_profile(request.floor).has_value()) {
            set_error(error, "NextLevel recognized an unsupported floor: " + std::to_string(request.floor));
            return false;
        }

        const auto capture_start = std::chrono::steady_clock::now();
        const cv::Mat image = m_task_context->capture();
        BlackFlowObservationRequest current_request = request;
        current_request.capture_us =
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - capture_start)
                .count();

        BlackFlowPerceptionSnapshot next;
        std::string perception_error;
        if (!m_map_source
                 ->recognize(image, current_request, next.observation, next.observed_facts, &perception_error)) {
            snapshot = std::move(next);
            set_error(
                error,
                "map_model_failed_after_next_level_floor: floor " + std::to_string(request.floor) + ": " +
                    perception_error);
            Log.warn(
                "BlackFlow map model failed after NextLevel floor recognition",
                "floor",
                request.floor,
                "error",
                perception_error);
            return false;
        }
        if (const auto action_points = recognize_action_points(image); action_points.has_value()) {
            next.run.action_points = *action_points;
            next.observation.hud_action_points = *action_points;
        }
        if (const auto movement = recognize_loaded_movement(image); movement.has_value()) {
            next.run.active_movement = *movement;
        }
        snapshot = std::move(next);
        return true;
    }
    catch (const std::exception& exception) {
        set_error(error, "BlackFlow map refresh failed: " + std::string(exception.what()));
        return false;
    }
    catch (...) {
        set_error(error, "BlackFlow map refresh failed: unknown exception");
        return false;
    }
}

bool BlackFlowTaskPort::preview(
    const MoveCandidate& candidate,
    const ViewportObservation& viewport,
    MovePreview& preview,
    bool& panel_open,
    std::string* error)
{
    panel_open = false;
    if (m_task_context == nullptr) {
        set_error(error, "BlackFlow ProcessTask context is not attached");
        return false;
    }
    const auto click_rect =
        viewport.clickable_rect(candidate.target, viewport.map_revision(), viewport.viewport_revision());
    if (!click_rect.has_value()) {
        set_error(error, "target node has no current viewport rectangle");
        return false;
    }
    // 缩到图标框中心一半再点，边缘落空会让预览面板弹不出来。
    if (!m_task_context->click(shrink_to_center(*click_rect, 0.5))) {
        set_error(error, "target node click failed");
        return false;
    }
    panel_open = true;

    if (!m_task_context->execute({ std::string(MovePreviewWaitTask) }, error)) {
        return false;
    }
    const std::string& matched = m_task_context->last_task();
    if (matched.ends_with(MovePreviewCannotEnterTask)) {
        preview.reachability = PreviewReachability::Blocked;
        preview.exact_action_point_cost = candidate.predicted_action_point_cost;
        return true;
    }
    if (!matched.ends_with(MovePreviewEnterTask)) {
        set_error(error, "move preview did not identify a reachable or blocked state");
        return false;
    }

    const std::shared_ptr<cv::Mat> matched_image = m_task_context->last_image();
    const cv::Mat image =
        matched_image != nullptr && !matched_image->empty() ? *matched_image : m_task_context->capture();
    const auto displayed_cost = recognize_integer(image, MovePreviewCostTask);
    if (!displayed_cost.has_value() || *displayed_cost > 0 || *displayed_cost < -9) {
        set_error(error, "move preview action point cost OCR failed");
        return false;
    }
    const auto displayed_name =
        recognize_text(image, MovePreviewDisplayedNameTask, BlackFlowNodeExecution.preview_names());
    if (!displayed_name.has_value()) {
        set_error(error, "move preview displayed name OCR failed");
        return false;
    }
    const auto displayed_type = BlackFlowNodeExecution.preview_node_type(*displayed_name);
    if (!displayed_type.has_value()) {
        set_error(error, "move preview displayed name has no node type mapping: " + *displayed_name);
        return false;
    }

    preview.reachability = PreviewReachability::Reachable;
    preview.exact_action_point_cost = -*displayed_cost;
    preview.displayed_type = *displayed_type;
    preview.displayed_name = *displayed_name;
    preview.identity_revealed = *displayed_type != NodeType::HideInvisible && *displayed_type != NodeType::HideBattle;
    return true;
}

MoveConfirmationStatus BlackFlowTaskPort::confirm(
    const MoveTransaction& transaction,
    EnteredPageObservation& entered_page,
    std::string* error)
{
    if (m_task_context == nullptr || transaction.stage() != MoveTransactionStage::Previewed ||
        !transaction.preview().has_value() || transaction.preview()->reachability != PreviewReachability::Reachable) {
        set_error(error, "move confirmation requires a reachable previewed transaction");
        return MoveConfirmationStatus::Failed;
    }
    if (!m_task_context->execute({ std::string(MovePreviewConfirmTask) }, error)) {
        return MoveConfirmationStatus::Failed;
    }
    const std::string confirmation_task = m_task_context->last_task();
    const MoveConfirmationTaskDisposition disposition = resolve_move_confirmation_task(
        confirmation_task,
        [this](std::string* cleanup_error) {
            if (m_inventory_cleanup == nullptr) {
                set_error(cleanup_error, "inventory cleanup component is not attached");
                return false;
            }
            return m_inventory_cleanup->run(*m_task_context, cleanup_error);
        },
        error);
    if (disposition == MoveConfirmationTaskDisposition::InventoryCleaned) {
        return MoveConfirmationStatus::InventoryCleaned;
    }
    if (disposition == MoveConfirmationTaskDisposition::NeedsDismiss) {
        return MoveConfirmationStatus::NeedsDismiss;
    }
    if (disposition == MoveConfirmationTaskDisposition::Failed) {
        return MoveConfirmationStatus::Failed;
    }
    // 成功末端单独执行，继续保留默认任务间隔和既有的 600 毫秒页面稳定等待。
    if (!m_task_context->execute({ std::string(MovePreviewConfirmCompletedTask) }, error)) {
        return MoveConfirmationStatus::Failed;
    }

    entered_page = {};
    if (transaction.preview()->identity_revealed) {
        return MoveConfirmationStatus::Succeeded;
    }
    return classify_entered_page(m_task_context->capture(), entered_page, error) ? MoveConfirmationStatus::Succeeded
                                                                                 : MoveConfirmationStatus::Failed;
}

void BlackFlowTaskPort::configure_diagnostics(const DiagnosticSettings& settings)
{
    if (m_map_source != nullptr) {
        m_map_source->configure_diagnostics(settings);
    }
}

bool BlackFlowTaskPort::persist_diagnostics(const DiagnosticArtifactRequest& request, std::string* error)
{
    if (m_map_source == nullptr) {
        set_error(error, "BlackFlow diagnostic persistence has no map observation source");
        return false;
    }
    return m_map_source->persist_diagnostics(request, error);
}

std::optional<int> BlackFlowTaskPort::recognize_action_points(const cv::Mat& image) const
{
    const auto value = recognize_integer(image, CurrentActionPointsTask);
    if (!value.has_value() || *value < 0 || *value > 64) {
        return std::nullopt;
    }
    return value;
}

bool BlackFlowTaskPort::classify_entered_page(
    const cv::Mat& image,
    EnteredPageObservation& observation,
    std::string* error) const
{
    const auto task = Task.get<OcrTaskInfo>(EnteredPageClassificationTask);
    if (task == nullptr) {
        set_error(error, "entered-page OCR task is missing");
        return false;
    }

    OCRer analyzer(image);
    analyzer.set_task_info(task);
    const auto results = analyzer.analyze();
    std::vector<std::string> matched_texts;
    if (results.has_value()) {
        matched_texts.reserve(results->size());
        for (const auto& result : *results) {
            matched_texts.emplace_back(result.text);
        }
    }
    observation = classify_entered_page_texts(std::move(matched_texts));
    if (observation.classified_type.has_value() || observation.classification_conflict) {
        return true;
    }

    if (!m_task_context->execute({ std::string(EnteredPageClassificationEncounterPrepareTask) }, error)) {
        return false;
    }
    const auto encounter_title = recognize_text(m_task_context->capture(), StageEncounterOcrTask);
    if (encounter_title.has_value()) {
        observation.matched_texts.emplace_back(*encounter_title);
        observation.classified_type = NodeType::Incident;
    }
    return true;
}
} // namespace asst::blackflow
