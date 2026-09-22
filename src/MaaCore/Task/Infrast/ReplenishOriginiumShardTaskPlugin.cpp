#include "ReplenishOriginiumShardTaskPlugin.h"

#include <string>

#include "Controller/Controller.h"
#include "OriginiumShardRecipe.h"
#include "Task/ProcessTask.h"

bool asst::ReplenishOriginiumShardTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskExtraInfo || details.get("subtask", std::string()) != "InfrastMfgTask") {
        return false;
    }

    if (details.at("what").as_string() == "ProductOfFacility" &&
        details.at("details").at("product").as_string() == "OriginStone") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::ReplenishOriginiumShardTaskPlugin::open_originium_shard_selector() const
{
    ProcessTask open_selector(*this, { "OpenOriginiumShardSelectorForReplenish" });
    if (!open_selector.run()) {
        return false;
    }

    ProcessTask choose_tab(*this, { "ChooseOriginiumShardTab" });
    if (choose_tab.run()) {
        return true;
    }

    ProcessTask close_selector(*this, { "Return" });
    close_selector.run();
    return false;
}

bool asst::ReplenishOriginiumShardTaskPlugin::close_originium_shard_selector() const
{
    return restore_mfg_product_details_page(*this);
}

bool asst::ReplenishOriginiumShardTaskPlugin::select_recipe(OriginiumShardRecipe recipe) const
{
    return run_originium_shard_recipe_task(*this, recipe);
}

bool asst::ReplenishOriginiumShardTaskPlugin::replenish_original()
{
    ProcessTask task(*this, { "ReplenishToMax" });
    return task.run();
}

bool asst::ReplenishOriginiumShardTaskPlugin::_run()
{
    if (!m_use_device) {
        return replenish_original();
    }

    if (!open_originium_shard_selector()) {
        if (!close_originium_shard_selector()) {
            return false;
        }
        return replenish_original();
    }

    // 切到配方页后统一识别材料；识别失败时仍回退到原有固源岩补货逻辑。
    const auto recipe = detect_originium_shard_recipe(ctrler()->get_image(), true);
    if (!recipe) {
        if (!close_originium_shard_selector()) {
            return false;
        }
        return replenish_original();
    }

    // 显式选择识别出的配方，确保装置耗尽后也能切回固源岩配方。
    if (!select_recipe(*recipe)) {
        if (!close_originium_shard_selector()) {
            return false;
        }
        return replenish_original();
    }

    return replenish_original();
}
