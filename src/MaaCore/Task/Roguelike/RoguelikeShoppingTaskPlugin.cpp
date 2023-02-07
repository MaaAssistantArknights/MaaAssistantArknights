#include "RoguelikeShoppingTaskPlugin.h"

#include "Controller.h"
#include "Vision/OcrWithFlagTemplImageAnalyzer.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Roguelike/RoguelikeShoppingConfig.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeShoppingTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    auto roguelike_name_opt = status()->get_properties(Status::RoguelikeTheme);
    if (!roguelike_name_opt) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    const std::string roguelike_name = std::move(roguelike_name_opt.value()) + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@TraderRandomShopping") {
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
    analyzer.set_task_info("RoguelikeTraderShopping", "RoguelikeTraderShoppingOcr");

    analyzer.set_image(ctrler()->get_image());
    if (!analyzer.analyze()) {
        return false;
    }

    bool no_longer_buy = status()->get_number(Status::RoguelikeTraderNoLongerBuy).value_or(0) ? true : false;

    std::string str_chars_info = status()->get_str(Status::RoguelikeCharOverview).value_or(json::value().to_string());
    json::value json_chars_info = json::parse(str_chars_info).value_or(json::value());

    std::unordered_map<battle::Role, size_t> map_roles_count;
    std::unordered_map<battle::Role, size_t> map_wait_promotion;
    size_t total_wait_promotion = 0;
    std::unordered_set<std::string> chars_list;
    for (auto& [name, json_info] : json_chars_info.as_object()) {
        int elite = static_cast<int>(json_info.get("elite", 0));
        int level = static_cast<int>(json_info.get("level", 0));
        Log.info(name, elite, level);

        // 等级太低的干员没必要为他专门买收藏品什么的
        if (level < 60) {
            continue;
        }

        chars_list.emplace(name);

        if (name == "阿米娅") {
            map_roles_count[battle::Role::Caster] += 1;
            map_roles_count[battle::Role::Warrior] += 1;
            if (elite == 1 && level == 70) {
                total_wait_promotion += 1;
                map_wait_promotion[battle::Role::Caster] += 1;
                map_wait_promotion[battle::Role::Warrior] += 1;
            }
        }
        else {
            battle::Role role = BattleData.get_role(name);
            map_roles_count[role] += 1;

            static const std::unordered_map<int, int> RarityPromotionLevel = {
                { 0, INT_MAX }, { 1, INT_MAX }, { 2, INT_MAX }, { 3, INT_MAX }, { 4, 60 }, { 5, 70 }, { 6, 80 },
            };
            int rarity = BattleData.get_rarity(name);
            if (elite == 1 && level >= RarityPromotionLevel.at(rarity)) {
                total_wait_promotion += 1;
                map_wait_promotion[role] += 1;
            }
        }
    }

    const auto& result = analyzer.get_result();
    bool bought = false;
    auto& all_goods = RoguelikeShopping.get_goods(status()->get_properties(Status::RoguelikeTheme).value());
    for (const auto& goods : all_goods) {
        if (need_exit()) {
            return false;
        }
        if (no_longer_buy && !goods.ignore_no_longer_buy) {
            continue;
        }

        auto find_it = ranges::find_if(result, [&](const TextRect& tr) -> bool {
            return tr.text.find(goods.name) != std::string::npos || goods.name.find(tr.text) != std::string::npos;
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
                bool role_mathced = false;
                for (const auto& role : goods.roles) {
                    if (map_wait_promotion[role] != 0) {
                        role_mathced = true;
                        break;
                    }
                }
                if (!role_mathced) {
                    Log.trace("Ready to buy", goods.name, ", but there is no one waiting for promotion, skip");
                    continue;
                }
            }
        }

        if (!goods.chars.empty()) {
            if (ranges::find_first_of(chars_list, goods.chars) == chars_list.cend()) {
                Log.trace("Ready to buy", goods.name, ", but there is no such character, skip");
                continue;
            }
        }

        // 这里仅点一下收藏品，原本的 ProcessTask 还会再点一下，但它是由 rect_move 的，保证不会点出去
        // 即 ProcessTask 多点的那一下会点到不影响的地方
        // 然后继续走 next 里确认 or 取消等等的逻辑
        Log.info("Ready to buy", goods.name);
        ctrler()->click(find_it->rect);
        bought = true;
        if (goods.no_longer_buy) {
            status()->set_number(Status::RoguelikeTraderNoLongerBuy, 1);
        }
        break;
    }

    if (!bought) {
        // 如果什么都没买，即使有商品，说明也是不需要买的，这里强制离开商店，后面让 ProcessTask 继续跑
        return ProcessTask(*this, { "RoguelikeTraderShoppingOver" }).run();
    }

    return true;
}
