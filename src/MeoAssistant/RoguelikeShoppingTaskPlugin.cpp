#include "RoguelikeShoppingTaskPlugin.h"

#include "OcrWithFlagTemplImageAnalyzer.h"
#include "Resource.h"
#include "Controller.h"
#include "ProcessTask.h"
#include "Logger.hpp"
#include "RuntimeStatus.h"

bool asst::RoguelikeShoppingTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart
        || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (details.at("details").at("task").as_string() == "Roguelike1TraderRandomShopping") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeShoppingTaskPlugin::_run()
{
    LogTraceFunction;

    OcrWithFlagTemplImageAnalyzer analyzer;
    analyzer.set_task_info("Roguelike1TraderShopping", "Roguelike1TraderShoppingOcr");

    analyzer.set_image(m_ctrler->get_image());
    if (!analyzer.analyze()) {
        return false;
    }

    bool no_longer_buy =
        m_status->get_number(RuntimeStatus::RoguelikeTraderNoLongerBuy).value_or(0)
        ? true : false;

    std::string str_chars_info =
        m_status->get_str(RuntimeStatus::RoguelikeCharOverview)
        .value_or(json::value().to_string());
    json::value json_chars_info = json::parse(str_chars_info).value_or(json::value());

    std::unordered_map<BattleRole, size_t> map_roles_count;
    std::unordered_set<std::string> chars_list;
    for (auto& [name, json_info] : json_chars_info.as_object()) {
        int elite = static_cast<int>(json_info.get("elite", 0));
        int level = static_cast<int>(json_info.get("level", 0));
        Log.info(name, elite, level);

        // 等级太低的干员没必要为他专门买收藏品什么的
        if (elite * 1000 + level < 1070) {
            continue;
        }

        chars_list.emplace(name);

        if (name == "阿米娅") {
            map_roles_count[BattleRole::Caster] += 1;
            map_roles_count[BattleRole::Warrior] += 1;
        }
        else {
            BattleRole role = Resrc.battle_data().get_role(name);
            map_roles_count[role] += 1;
        }
    }

    const auto& result = analyzer.get_result();
    const auto& order_goods_list = Resrc.roguelike_shopping().get_goods();
    bool bought = false;
    for (const auto& goods : order_goods_list) {
        if (need_exit()) {
            return false;
        }
        if (no_longer_buy && !goods.ignore_no_longer_buy) {
            continue;
        }

        auto find_it = std::find_if(result.cbegin(), result.cend(),
            [&](const TextRect& tr) -> bool {
                return tr.text.find(goods.name) != std::string::npos ||
                    goods.name.find(tr.text) != std::string::npos;
            });
        if (find_it == result.cend()) {
            continue;
        }

        if (!goods.roles.empty()) {
            bool role_mathced = false;
            for (const auto& role : goods.roles) {
                if (map_roles_count[role] != 0) {
                    role_mathced = true;
                    break;
                }
            }
            if (!role_mathced) {
                Log.trace("Ready to buy", goods.name,
                    ", but there is no such professional operator, skip");
                continue;
            }
        }

        if (!goods.chars.empty()) {
            auto iter = std::find_first_of(chars_list.cbegin(), chars_list.cend(),
                goods.chars.cbegin(), goods.chars.cend());
            if (iter == chars_list.cend()) {
                Log.trace("Ready to buy", goods.name,
                    ", but there is no such character, skip");
                continue;
            }
        }

        // 这里仅点一下收藏品，原本的 ProcessTask 还会再点一下，但它是由 rect_move 的，保证不会点出去
        // 即 ProcessTask 多点的那一下会点到不影响的地方
        // 然后继续走 next 里确认 or 取消等等的逻辑
        Log.info("Ready to buy", goods.name);
        m_ctrler->click(find_it->rect);
        bought = true;
        if (goods.no_longer_buy) {
            m_status->set_number(RuntimeStatus::RoguelikeTraderNoLongerBuy, 1);
        }
        break;
    }

    if (!bought) {
        // 如果什么都没买，即使有商品，说明也是不需要买的，这里强制离开商店，后面让 ProcessTask 继续跑
        return ProcessTask(*this, { "Roguelike1TraderShoppingOver" }).run();
    }

    return true;
}
