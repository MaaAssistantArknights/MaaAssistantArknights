#include "RoguelikeShoppingTaskPlugin.h"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Roguelike/RoguelikeShoppingConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"

bool asst::RoguelikeShoppingTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (m_roguelike_config->get_theme().empty()) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    const std::string roguelike_name = m_roguelike_config->get_theme() + "@";
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

    OCRer analyzer;
    analyzer.set_task_info("RoguelikeTraderShoppingOcr");
    auto image = ctrler()->get_image();
    analyzer.set_image(image);
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

    bool bought = false;
    auto& all_goods = RoguelikeShopping.get_goods(m_roguelike_theme);
    std::vector<std::string> all_foldartal = m_roguelike_theme == "Sami"
                                                 ? Task.get<OcrTaskInfo>("Sami@Roguelike@FoldartalGainOcr")->text
                                                 : std::vector<std::string>();
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
                    if (map_wait_promotion[role] != 0) {
                        role_matched = true;
                        break;
                    }
                }
                if (!role_matched) {
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
        if (m_roguelike_theme == "Sami") {

            auto iter = std::find(all_foldartal.begin(), all_foldartal.end(), goods.name);
            if (iter != all_foldartal.end()) {
                std::string overview_str =
                    status()->get_str(Status::RoguelikeFoldartalOverview).value_or(json::value().to_string());

                auto& overview = json::parse(overview_str).value_or(json::value()).as_array();
                // 把goods.name存到密文板overview里
                overview.push_back(goods.name);
                status()->set_str(Status::RoguelikeFoldartalOverview, overview.to_string());
            }
        }
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
