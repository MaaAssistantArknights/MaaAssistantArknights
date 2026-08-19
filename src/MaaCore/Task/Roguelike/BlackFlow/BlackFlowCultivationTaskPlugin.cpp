#include "BlackFlowCultivationTaskPlugin.h"

#include <algorithm>
#include <cstdlib>
#include <string_view>
#include <utility>

#include "Config/Roguelike/BlackFlow/BlackFlowNodeExecutionConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"
#include "Vision/Roguelike/RoguelikeParameterAnalyzer.h"

namespace asst::blackflow
{
namespace
{
constexpr std::string_view EnterTask = "BlackFlow@Roguelike@CultivateEnter";
constexpr std::string_view SellDecisionTask = "BlackFlow@Roguelike@CultivateSellDecision";
constexpr std::string_view SellAction = "BlackFlow@Roguelike@CultivateSellAction";
constexpr std::string_view SellItemsTask = "BlackFlow@Roguelike@CultivateSellItems";
constexpr std::string_view SellConfirmEntry = "BlackFlow@Roguelike@CultivateSellConfirm-Enter";
constexpr std::string_view ToggleToBuyTask = "BlackFlow@Roguelike@CultivateToggleToBuy";
constexpr std::string_view BuyDecisionTask = "BlackFlow@Roguelike@CultivateBuyDecision";
constexpr std::string_view BuyAction = "BlackFlow@Roguelike@CultivateBuyAction";
constexpr std::string_view ShelfSeedTask = "BlackFlow@Roguelike@CultivateShelfSeed";
constexpr std::string_view WalletTask = "BlackFlow@Roguelike@CultivateWallet";
constexpr std::string_view HeldSeedCountTask = "BlackFlow@Roguelike@CultivateHeldSeedCount";
constexpr std::string_view BuyConfirmEntry = "BlackFlow@Roguelike@CultivateBuyConfirm-Enter";
constexpr std::string_view BuyConfirmTask = "BlackFlow@Roguelike@CultivateBuyConfirm";
constexpr std::string_view RefreshEntry = "BlackFlow@Roguelike@CultivateRefresh-Enter";
constexpr std::string_view RefreshCompletedTask = "BlackFlow@Roguelike@CultivateRefreshCompleted";
constexpr std::string_view StartCultivationEntry = "BlackFlow@Roguelike@CultivateStartButton-Enter";
constexpr std::string_view LeaveEntry = "BlackFlow@Roguelike@CultivateLeave-Enter";
constexpr std::string_view AtMostTask = "BlackFlow@Roguelike@CultivateAtMost";
constexpr std::string_view HarvestReadyTask = "BlackFlow@Roguelike@CultivateHarvestReady";
constexpr std::string_view HarvestItemsTask = "BlackFlow@Roguelike@CultivateHarvestItems";
constexpr std::string_view CultivationResultTask = "BlackFlow@Roguelike@CultivationResult";
constexpr std::string_view CompletionAction = "BlackFlow@Roguelike@NodeCompletionAction";
constexpr std::string_view NoSeedCompletionEntry = "BlackFlow@Roguelike@Event-BabyNoSeed-Enter";
constexpr std::string_view CultivatedCompletionEntry = "BlackFlow@Roguelike@Event-BabyCultivationCompleted-Enter";

constexpr int SeedPrice = 4;
constexpr int RefreshPriceStep = 4;
constexpr int MaxRefreshTimes = 4;
constexpr int SameShelfSlotTolerance = 20;
constexpr int AtMostClickTimes = 3;
constexpr int RepeatedClickInterval = 100;
} // namespace

bool BlackFlowCultivationTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string task = details.get("details", "task", "");
    if (msg == AsstMsg::SubTaskStart) {
        if (task == EnterTask) {
            m_pending = PendingWork::Enter;
            return true;
        }
        if (task == SellDecisionTask) {
            m_pending = PendingWork::SellDecision;
            return true;
        }
        if (task == BuyDecisionTask) {
            m_pending = PendingWork::BuyDecision;
            m_pending_details = details;
            return true;
        }
        if (task == RefreshCompletedTask) {
            m_pending = PendingWork::RefreshCompleted;
            return true;
        }
        if (task == AtMostTask) {
            m_pending = PendingWork::ClickAtMost;
            return true;
        }
        if (task == HarvestReadyTask) {
            m_pending = PendingWork::ReadHarvest;
            return true;
        }
    }
    if (msg == AsstMsg::SubTaskCompleted && task == BuyConfirmTask) {
        m_pending = PendingWork::BuyConfirmed;
        return true;
    }
    if (msg == AsstMsg::SubTaskCompleted && task == CultivationResultTask) {
        m_pending = PendingWork::ApplyCultivationResult;
        m_pending_details = details;
        return true;
    }
    return false;
}

void BlackFlowCultivationTaskPlugin::reset_in_run_variables()
{
    m_pending = PendingWork::None;
    m_pending_details = {};
    m_pending_purchase.reset();
    m_purchased_shelf_rects.clear();
    m_refresh_count = 0;
    m_cultivated_animals = 0;
    m_cultivated_animal_types.clear();
}

bool BlackFlowCultivationTaskPlugin::already_purchased(const Rect& rect) const
{
    const int center_x = rect.x + rect.width / 2;
    const int center_y = rect.y + rect.height / 2;
    return std::ranges::any_of(m_purchased_shelf_rects, [&](const Rect& bought) {
        return std::abs(center_x - (bought.x + bought.width / 2)) <= SameShelfSlotTolerance &&
               std::abs(center_y - (bought.y + bought.height / 2)) <= SameShelfSlotTolerance;
    });
}

bool BlackFlowCultivationTaskPlugin::_run()
{
    LogTraceFunction;
    const PendingWork work = m_pending;
    m_pending = PendingWork::None;

    if (work == PendingWork::Enter) {
        m_pending_purchase.reset();
        m_purchased_shelf_rects.clear();
        m_refresh_count = 0;
        m_cultivated_animals = 0;
        m_cultivated_animal_types.clear();
        return true;
    }

    if (work == PendingWork::SellDecision) {
        Task.set_task_base(std::string(SellAction), std::string(ToggleToBuyTask));
        const auto items = recognize(ctrler()->get_image(), std::string(SellItemsTask));
        if (!items.empty()) {
            ctrler()->click(items.front().rect);
            Task.set_task_base(std::string(SellAction), std::string(SellConfirmEntry));
        }
        return true;
    }

    if (work == PendingWork::BuyDecision) {
        const cv::Mat image = ctrler()->get_image();
        const int wallet = read_number(image, std::string(WalletTask));
        const auto shelf_seeds = recognize(image, std::string(ShelfSeedTask));
        const auto available =
            std::ranges::find_if(shelf_seeds, [this](const TextRect& seed) { return !already_purchased(seed.rect); });

        if (available != shelf_seeds.end() && wallet >= SeedPrice) {
            ctrler()->click(available->rect);
            m_pending_purchase = available->rect;
            Task.set_task_base(std::string(BuyAction), std::string(BuyConfirmEntry));
            return true;
        }

        const int refresh_price = RefreshPriceStep * (m_refresh_count + 1);
        if (available == shelf_seeds.end() && m_refresh_count < MaxRefreshTimes &&
            wallet >= refresh_price + SeedPrice) {
            Task.set_task_base(std::string(BuyAction), std::string(RefreshEntry));
            return true;
        }

        const int held_seeds = read_number(image, std::string(HeldSeedCountTask));
        if (held_seeds > 0) {
            Task.set_task_base(std::string(BuyAction), std::string(StartCultivationEntry));
        }
        else {
            m_cultivated_animals = 0;
            apply_cultivation_result(std::string(NoSeedCompletionEntry));
            Task.set_task_base(std::string(BuyAction), std::string(LeaveEntry));
        }
        return true;
    }

    if (work == PendingWork::BuyConfirmed) {
        if (m_pending_purchase.has_value()) {
            m_purchased_shelf_rects.emplace_back(*m_pending_purchase);
            m_pending_purchase.reset();
            Log.info("BlackFlow cultivation shelf purchased", "slots", m_purchased_shelf_rects.size());
        }
        return true;
    }

    if (work == PendingWork::RefreshCompleted) {
        m_pending_purchase.reset();
        m_purchased_shelf_rects.clear();
        ++m_refresh_count;
        return true;
    }

    if (work == PendingWork::ClickAtMost) {
        if (const auto hit = get_hit_detail<Matcher::Result>(); hit != nullptr) {
            int times = AtMostClickTimes;
            while (times-- > 0) {
                ctrler()->click(hit->rect);
                sleep(RepeatedClickInterval);
            }
        }
        return true;
    }

    if (work == PendingWork::ReadHarvest) {
        const auto items = recognize(ctrler()->get_image(), std::string(HarvestItemsTask));
        m_cultivated_animals = static_cast<int>(items.size());
        m_cultivated_animal_types.clear();
        for (const TextRect& item : items) {
            if (const auto type = cultivated_animal_type_from_name(item.text); type.has_value()) {
                m_cultivated_animal_types.emplace_back(*type);
            }
        }
        Log.info("BlackFlow cultivation harvest recognized", "count", m_cultivated_animals);
        return true;
    }

    if (work == PendingWork::ApplyCultivationResult) {
        apply_cultivation_result(std::string(CultivatedCompletionEntry));
        return true;
    }
    return true;
}

std::vector<TextRect> BlackFlowCultivationTaskPlugin::recognize(const cv::Mat& image, const std::string& task) const
{
    const auto task_info = Task.get<OcrTaskInfo>(task);
    if (task_info == nullptr) {
        return {};
    }

    OCRer analyzer(image);
    analyzer.set_task_info(task_info);
    analyzer.set_required(task_info->text);
    const auto results = analyzer.analyze();
    return results.has_value() ? std::move(*results) : std::vector<TextRect> {};
}

int BlackFlowCultivationTaskPlugin::read_number(const cv::Mat& image, const std::string& task) const
{
    RoguelikeParameterAnalyzer analyzer(image);
    return analyzer.get_number(image, task);
}

void BlackFlowCultivationTaskPlugin::bind_completion(const std::string& task) const
{
    Task.set_task_base(std::string(CompletionAction), task);
}

void BlackFlowCultivationTaskPlugin::apply_cultivation_result(const std::string& completion_task)
{
    if (m_session == nullptr) {
        return;
    }

    const NodeTaskResult* result = BlackFlowNodeExecution.get_task_result(std::string(CultivationResultTask));
    if (result == nullptr) {
        return;
    }

    m_session->set_cultivated_animal_types(m_cultivated_animal_types);

    json::value details = m_pending_details;
    details["details"]["result"] = json::object { { "text", std::to_string(m_cultivated_animals) } };

    std::string error;
    if (!m_session->apply_node_task_result(*result, details, &error)) {
        Log.error("BlackFlow cultivation result callback failed", error);
        return;
    }

    bind_completion(completion_task);
    report_outputs();
}
} // namespace asst::blackflow
