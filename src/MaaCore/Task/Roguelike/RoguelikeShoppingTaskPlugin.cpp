#include "RoguelikeShoppingTaskPlugin.h"

#include <array>

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Roguelike/RoguelikeShoppingConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Task/Roguelike/BlackFlow/BlackFlowSession.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"
#include "Vision/RegionOCRer.h"

bool asst::RoguelikeShoppingTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string task = details.get("details", "task", "");
    const bool strategy_shopping = RoguelikeShopping.strategy_shopping_enabled(m_config->get_theme());
    if (strategy_shopping && m_pending_purchase && m_blackflow_session) {
        if (msg == AsstMsg::SubTaskCompleted && task.ends_with("Roguelike@TraderRandomShoppingConfirm")) {
            m_pending = PendingWork::PurchaseConfirmed;
            return true;
        }
        if (msg == AsstMsg::SubTaskStart &&
            (task.ends_with("Roguelike@TraderRandomShoppingCancel") || task.ends_with("Roguelike@StageTraderLeave") ||
             task.ends_with("Roguelike@StageTraderLeaveConfirm"))) {
            m_pending = PendingWork::ClearPurchase;
            return true;
        }
    }
    if (msg != AsstMsg::SubTaskStart || !task.ends_with("Roguelike@TraderRandomShopping")) {
        return false;
    }
    if (m_config->get_mode() == RoguelikeMode::Investment && !m_config->get_invest_with_more_score()) {
        return false;
    }
    if (m_config->get_mode() == RoguelikeMode::Collectible && !m_config->get_collectible_mode_shopping()) {
        return false;
    }
    if (strategy_shopping) {
        m_pending = PendingWork::Buy;
    }
    return true;
}

void asst::RoguelikeShoppingTaskPlugin::reset_in_run_variables()
{
    m_pending = PendingWork::None;
    m_pending_purchase.reset();
}

bool asst::RoguelikeShoppingTaskPlugin::_run()
{
    LogTraceFunction;

    const bool strategy_shopping = RoguelikeShopping.strategy_shopping_enabled(m_config->get_theme());
    if (strategy_shopping) {
        const PendingWork work = std::exchange(m_pending, PendingWork::None);
        if (work == PendingWork::PurchaseConfirmed) {
            const auto purchase = std::exchange(m_pending_purchase, std::nullopt);
            std::string error;
            if (purchase && !m_blackflow_session->apply_shopping_purchase(*purchase, &error)) {
                LogError << __FUNCTION__ << "BlackFlow shopping purchase failed" << error;
                return false;
            }
            return true;
        }
        if (work == PendingWork::ClearPurchase) {
            m_pending_purchase.reset();
            return true;
        }
        if (work != PendingWork::Buy) {
            return true;
        }
        m_pending_purchase.reset();
    }

    buy_once();
    const auto& theme = m_config->get_theme();
    if ((theme == RoguelikeTheme::Sami || theme == RoguelikeTheme::Sarkaz) &&
        // 界园可能没有免费刷新，先不进这里
        m_config->get_mode() == RoguelikeMode::Exp) {
        // 点击刷新
        sleep(500);
        bool ret = ProcessTask(*this, { theme + "@Roguelike@StageTraderRefresh" }).run();
        ret = ret && ProcessTask(*this, { theme + "@Roguelike@StageTraderRefreshConfirm" })
                         .set_retry_times(RetryTimesDefault)
                         .run();
        if (ret) {
            buy_once();
        }
    }

    sleep(1000);

    return true;
}

bool asst::RoguelikeShoppingTaskPlugin::buy_once()
{
    LogTraceFunction;

    const auto& theme = m_config->get_theme();
    const bool strategy_shopping = RoguelikeShopping.strategy_shopping_enabled(theme);

    if (strategy_shopping && m_blackflow_session) {
        // 同店确认购买后事实可能变化，下一笔购买重新选表。
        const auto shopping = m_blackflow_session->shopping_rule();
        auto& status = m_config->status();
        status.shopping_buy_table = shopping ? shopping->get().buy_table : "default";
        status.shopping_sell_table = shopping ? shopping->get().sell_table : std::string();
    }

    auto image = ctrler()->get_image();
    OCRer analyzer(image);
    analyzer.set_task_info("RoguelikeTraderShoppingOcr");
    if (!analyzer.analyze()) {
        return false;
    }

    bool no_longer_buy = m_config->status().trader_no_longer_buy;

    std::unordered_map<battle::Role, size_t> map_roles_count;
    std::unordered_map<battle::Role, std::array<size_t, 6>> map_wait_promotion;
    size_t total_wait_promotion = 0;
    std::unordered_set<std::string> chars_list;
    for (const auto& [name, oper] : m_config->status().opers) {
        int elite = oper.elite;
        int level = oper.level;
        Log.info(name, elite, level);

        // 等级太低的干员没必要为他专门买收藏品什么的
        if (level < 60) {
            continue;
        }

        chars_list.emplace(name);

        if (name == "阿米娅") {
            map_roles_count[battle::Role::Caster] += 1;
            map_roles_count[battle::Role::Warrior] += 1;
            map_roles_count[battle::Role::Medic] += 1;
            if (elite == 1 && level == 70) {
                total_wait_promotion += 1;
                map_wait_promotion[battle::Role::Caster][5 - 1] += 1;
                map_wait_promotion[battle::Role::Warrior][5 - 1] += 1;
                map_wait_promotion[battle::Role::Medic][5 - 1] += 1;
            }
        }
        else {
            battle::Role role = BattleData.get_first_role(name);
            map_roles_count[role] += 1;

            static const std::unordered_map<int, int> RarityPromotionLevel = {
                { 0, INT_MAX }, { 1, INT_MAX }, { 2, INT_MAX }, { 3, INT_MAX }, { 4, 60 }, { 5, 70 }, { 6, 80 },
            };
            int rarity = BattleData.get_rarity(battle::Role::Unknown, name);
            if (elite == 1 && level >= RarityPromotionLevel.at(rarity)) {
                total_wait_promotion += 1;
                map_wait_promotion[role][rarity - 1] += 1;
            }
        }
    }

    const auto& raw_result = analyzer.get_result();
    std::vector<TextRect> result;
    Matcher matcher_analyzer;
    matcher_analyzer.set_image(image);
    matcher_analyzer.set_task_info("RoguelikeTraderShopping");
    for (auto& item : raw_result) {
        matcher_analyzer.set_roi(item.rect.move({ -20, 130, 200, 80 }));
        if (matcher_analyzer.analyze()) {
            result.emplace_back(item);
        }
    }

    // bool bought = false;
    const auto& all_goods = strategy_shopping
                               ? RoguelikeShopping.get_goods(theme, m_config->status().shopping_buy_table)
                               : RoguelikeShopping.get_goods(theme);
    const auto wallet = strategy_shopping ? read_wallet(image) : std::nullopt;
    std::vector<std::string> all_foldartal = m_config->get_theme() == RoguelikeTheme::Sami
                                                 ? Task.get<OcrTaskInfo>("Sami@Roguelike@FoldartalGainOcr")->text
                                                 : std::vector<std::string>();
    for (const auto& goods : all_goods) {
        if (need_exit()) {
            return false;
        }
        if (no_longer_buy && !goods.ignore_no_longer_buy) {
            continue;
        }

        // 在萨米肉鸽刷坍缩范式时不再购买会减少坍缩值的藏品
        if (m_config->get_mode() == RoguelikeMode::CLP_PDS && goods.decrease_collapse) {
            continue;
        }

        auto find_it = std::ranges::find_if(result, [&](const TextRect& tr) -> bool {
            return tr.text.find(goods.name) != std::string::npos || goods.name.find(tr.text) != std::string::npos;
        });
        if (find_it == result.cend()) {
            continue;
        }

        if (strategy_shopping && goods.price && wallet && *goods.price > *wallet) {
            LogTrace << __FUNCTION__ << "Ready to buy" << goods.name << "but the wallet is not enough, skip"
                     << *goods.price << *wallet;
            continue;
        }

        if (!goods.roles.empty()) {
            bool role_matched = false;
            for (const auto& role : goods.roles) {
                if (map_roles_count[role] != 0) {
                    role_matched = true;
                    break;
                }
            }
            if (!role_matched) {
                Log.trace("Ready to buy", goods.name, ", but there is no such professional operator, skip");
                continue;
            }
        }

        if (goods.promotion != 0) {
            if (total_wait_promotion == 0) {
                Log.trace("Ready to buy", goods.name, ", but there is no one waiting for promotion, skip");
                continue;
            }
            if (!goods.roles.empty()) {
                bool role_matched = false;
                for (const auto& role : goods.roles) {
                    size_t sum_wait_promotion = 0;
                    for (int rarity = 0; rarity < goods.promotion_rarity; ++rarity) {
                        sum_wait_promotion += map_wait_promotion[role][rarity];
                    }
                    if (sum_wait_promotion != 0) {
                        role_matched = true;
                        break;
                    }
                }
                if (!role_matched) {
                    Log.trace("Ready to buy", goods.name, ", but there is no one waiting for promotion, skip");
                    continue;
                }
            }
            else {
                size_t sum_wait_promotion = 0;
                for (const auto& [role, arr] : map_wait_promotion) {
                    for (int rarity = 0; rarity < goods.promotion_rarity; ++rarity) {
                        sum_wait_promotion += arr[rarity];
                    }
                }
                if (sum_wait_promotion == 0) {
                    Log.trace("Ready to buy", goods.name, ", but there is no one waiting for promotion, skip");
                    continue;
                }
            }
        }

        if (!goods.chars.empty()) {
            if (std::ranges::find_first_of(chars_list, goods.chars) == chars_list.cend()) {
                Log.trace("Ready to buy", goods.name, ", but there is no such character, skip");
                continue;
            }
        }

        // 这里仅点一下收藏品，原本的 ProcessTask 还会再点一下，但它是由 rect_move
        // 的，保证不会点出去 即 ProcessTask 多点的那一下会点到不影响的地方 然后继续走 next 里确认
        // or 取消等等的逻辑
        Log.info("Ready to buy", goods.name);
        ctrler()->click(find_it->rect);
        if (strategy_shopping && m_blackflow_session) {
            m_pending_purchase = goods.name;
        }
        // bought = true;
        if (m_config->get_theme() == RoguelikeTheme::Sami) {
            auto iter = std::find(all_foldartal.begin(), all_foldartal.end(), goods.name);
            if (iter != all_foldartal.end()) {
                // 把goods.name存到密文板overview里
                m_config->status().foldartal_list.emplace_back(goods.name);
            }
        }
        // 把goods.name存到已获得藏品里
        m_config->status().collections.emplace_back(goods.name);
        if (goods.no_longer_buy) {
            m_config->status().trader_no_longer_buy = true;
        }
        break;
    }
    /*
    if (!bought) {
        // 如果什么都没买，即使有商品，说明也是不需要买的，这里强制离开商店，后面让 ProcessTask
    继续跑 return ProcessTask(*this, { "RoguelikeTraderShoppingOver" }).run();
    }
    */
    return true;
}

std::optional<int> asst::RoguelikeShoppingTaskPlugin::read_wallet(const cv::Mat& image) const
{
    const std::string themed_task = m_config->get_theme() + "@Roguelike@StageTraderInvest-Wallet";
    const std::string task_name = Task.get(themed_task) != nullptr ? themed_task : "Roguelike@StageTraderInvest-Wallet";
    RegionOCRer ocr(image);
    ocr.set_task_info(task_name);
    ocr.set_use_raw(false);
    ocr.set_replace(Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map);
    int wallet = 0;
    if (!ocr.analyze() || !utils::chars_to_number(ocr.get_result().text, wallet)) {
        return std::nullopt;
    }
    return wallet;
}
