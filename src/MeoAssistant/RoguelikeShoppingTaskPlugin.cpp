#include "RoguelikeShoppingTaskPlugin.h"

#include "OcrWithFlagTemplImageAnalyzer.h"
#include "Resource.h"
#include "Controller.h"
#include "ProcessTask.h"
#include "Logger.hpp"

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
    const auto& result = analyzer.get_result();
    const auto& order_goods_list = Resrc.roguelike_shopping().get_goods();

    bool bought = false;
    for (const auto& goods : order_goods_list) {
        if (need_exit()) {
            return false;
        }

        if (goods.restriction != BattleRole::Unknown) {
            // TODO
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
        // 这里仅点一下收藏品，原本的 ProcessTask 还会再点一下，但它是由 rect_move 的，保证不会点出去
        // 即 ProcessTask 多点的那一下会点到不影响的地方
        // 然后继续走 next 里确认 or 取消等等的逻辑
        Log.info("Ready to buy", goods.name);
        m_ctrler->click(find_it->rect);
        bought = true;
        break;
    }

    if (!bought) {
        // 如果什么都没买，即使有商品，说明也是不需要买的，这里强制离开商店，后面让 ProcessTask 继续跑
        return ProcessTask(*this, { "Roguelike1TraderShoppingOver" }).run();
    }

    return true;
}
