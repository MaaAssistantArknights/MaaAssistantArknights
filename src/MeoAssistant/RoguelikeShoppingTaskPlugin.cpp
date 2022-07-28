#include "RoguelikeShoppingTaskPlugin.h"

#include "OcrWithFlagTemplImageAnalyzer.h"
#include "Resource.h"
#include "Controller.h"
#include "ProcessTask.h"

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
    OcrWithFlagTemplImageAnalyzer analyzer;
    analyzer.set_task_info("Roguelike1TraderShopping", "Roguelike1TraderShoppingOcr");

    analyzer.set_image(m_ctrler->get_image());
    if (!analyzer.analyze()) {
        return false;
    }
    const auto& result = analyzer.get_result();
    const auto& order_goods_list = Resrc.roguelike_shopping().get_goods();

    for (const auto& goods : order_goods_list) {
        if (goods.restriction != BattleRole::Unknown) {
            // TODO
            continue;
        }
        auto find_it = std::find_if(result.cbegin(), result.cend(),
            [&](const TextRect& tr) -> bool {
                return tr.text == goods.name;
            });
        if (find_it == result.cend()) {
            continue;
        }
        m_ctrler->click(find_it->rect);
        bool confirmed = ProcessTask(*this, { "Roguelike1TraderShoppingConfirm" })
            .set_retry_times(5)
            .run();

        if (!confirmed) {
            return false;
        }
    }

    return true;
}
