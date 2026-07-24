#include "BlackFlowTaskPort.h"

#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "BlackFlowMovementRecognition.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/AbstractTask.h"
#include "Task/ProcessTask.h"
#include "Utils/StringMisc.hpp"
#include "Vision/OCRer.h"

namespace asst::blackflow
{
namespace
{
constexpr std::string_view CurrentActionPointsTask = "BlackFlow@Roguelike@CurrentActionPoints";
constexpr std::string_view MovePreviewEnterTask = "BlackFlow@Roguelike@MovePreviewEnter";
constexpr std::string_view MovePreviewCannotEnterTask = "BlackFlow@Roguelike@MovePreviewCannotEnter";
constexpr std::string_view MovePreviewCostTask = "BlackFlow@Roguelike@MovePreviewCost";
constexpr std::string_view MovePreviewConfirmTask = "BlackFlow@Roguelike@MovePreviewConfirm";
constexpr std::string_view EnteredPageClassificationTask = "BlackFlow@Roguelike@EnteredPageClassification";

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
} // namespace

class BlackFlowTaskPort::ProcessTaskContext final : public AbstractTask
{
public:
    ProcessTaskContext(const AsstCallback& callback, Assistant* inst, std::string_view task_chain) :
        AbstractTask(callback, inst, task_chain)
    {
        set_retry_times(0);
    }

    bool execute(std::vector<std::string> tasks, std::string* error)
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

    [[nodiscard]] const std::string& last_task() const noexcept { return m_last_task; }

    [[nodiscard]] std::shared_ptr<cv::Mat> last_image() const noexcept { return m_last_image; }

    [[nodiscard]] cv::Mat capture() const { return ctrler()->get_image(); }

    bool click(const Rect& rect) const { return ctrler()->click(rect); }

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

EnteredPageObservation classify_entered_page_texts(std::vector<std::string> matched_texts)
{
    std::set<std::string> texts(
        std::make_move_iterator(matched_texts.begin()),
        std::make_move_iterator(matched_texts.end()));
    EnteredPageObservation observation;
    observation.matched_texts.assign(texts.begin(), texts.end());

    const bool final = texts.contains("险路尽头");
    const bool shop = texts.contains("前瞻性投资系统") && texts.contains("刷新");
    const bool scrap_shop = texts.contains("机械师的园圃");
    const bool emergency_aid = texts.size() == 1 && texts.contains("刷新");
    const int classifications = static_cast<int>(final) + static_cast<int>(shop) + static_cast<int>(scrap_shop) +
                                static_cast<int>(emergency_aid);
    if (classifications > 1) {
        observation.classification_conflict = true;
    }
    else if (final) {
        observation.classified_type = NodeType::Final;
    }
    else if (shop) {
        observation.classified_type = NodeType::Shop;
    }
    else if (scrap_shop) {
        observation.classified_type = NodeType::ScrapShop;
    }
    else if (emergency_aid) {
        observation.classified_type = NodeType::Employ;
    }
    return observation;
}

BlackFlowTaskPort::BlackFlowTaskPort(
    const AsstCallback& callback,
    Assistant* inst,
    std::string_view task_chain,
    std::shared_ptr<IBlackFlowMapObservationSource> map_source) :
    m_task_context(std::make_unique<ProcessTaskContext>(callback, inst, task_chain)),
    m_map_source(std::move(map_source))
{
}

BlackFlowTaskPort::~BlackFlowTaskPort() = default;

bool BlackFlowTaskPort::refresh(
    const BlackFlowObservationRequest& request,
    BlackFlowPerceptionSnapshot& snapshot,
    std::string* error)
{
    if (m_map_source == nullptr || m_task_context == nullptr) {
        set_error(error, "BlackFlow map observation source is not attached");
        return false;
    }

    const cv::Mat image = m_task_context->capture();
    BlackFlowPerceptionSnapshot next;
    if (!m_map_source->recognize(image, request, next.observation, next.observed_facts, error)) {
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
    if (!m_task_context->click(*click_rect)) {
        set_error(error, "target node click failed");
        return false;
    }
    panel_open = true;

    if (!m_task_context->execute(
            { std::string(MovePreviewEnterTask), std::string(MovePreviewCannotEnterTask) },
            error)) {
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
    preview.reachability = PreviewReachability::Reachable;
    preview.exact_action_point_cost = -*displayed_cost;
    return true;
}

bool BlackFlowTaskPort::confirm(
    const MoveTransaction& transaction,
    EnteredPageObservation& entered_page,
    std::string* error)
{
    if (m_task_context == nullptr || transaction.stage() != MoveTransactionStage::Previewed ||
        !transaction.preview().has_value() || transaction.preview()->reachability != PreviewReachability::Reachable) {
        set_error(error, "move confirmation requires a reachable previewed transaction");
        return false;
    }
    if (!m_task_context->execute({ std::string(MovePreviewConfirmTask) }, error)) {
        return false;
    }
    return classify_entered_page(m_task_context->capture(), entered_page, error);
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
    observation = {};
    if (!results.has_value()) {
        return true;
    }

    std::vector<std::string> matched_texts;
    matched_texts.reserve(results->size());
    for (const auto& result : *results) {
        matched_texts.emplace_back(result.text);
    }
    observation = classify_entered_page_texts(std::move(matched_texts));
    return true;
}
} // namespace asst::blackflow
