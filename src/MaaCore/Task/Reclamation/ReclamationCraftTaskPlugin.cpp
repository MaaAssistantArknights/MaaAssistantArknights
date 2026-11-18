#include "ReclamationCraftTaskPlugin.h"

#include "Config/Miscellaneous/ReclamationToolConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

bool asst::ReclamationCraftTaskPlugin::load_params(const ReclamationConfig::Params& params)
{
    m_craft_task_name = params.theme + "@RA@PIS-CraftTool";
    return params.mode == ReclamationMode { TalesMode::ProsperityInSave };
}

bool asst::ReclamationCraftTaskPlugin::verify(const AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string& task_name = details.get("details", "task", "");
    return task_name == m_craft_task_name;
}

bool asst::ReclamationCraftTaskPlugin::_run()
{
    LogTraceFunction;

    // 同一次进入组装台的所有批次使用同一份配置。
    const auto params = m_config->get_params();
    const auto& theme = params->theme;
    const std::string select_prefix = inst_string() + "@" + theme + "@RA@PIS-";
    for (const std::string& tool_to_craft : params->tools_to_craft) {
        Task.get<OcrTaskInfo>(select_prefix + "ClickTool")->text =
            ReclamationToolConfig::get_instance().names_for(tool_to_craft);

        bool insufficient_materials = false;
        for (int batch = 0; !need_exit() && !insufficient_materials && batch < params->num_craft_batches; ++batch) {
            // Step 1: 选择要组装的道具, 若没有找到则在组装台界面向下滑动屏幕后再次检测, 直到识别到要组装的道具后点击
            ProcessTask(*this, { select_prefix + "SelectTool" }).run();
            sleep(500);

            int craft_amount = 0;
            while (!need_exit() && !insufficient_materials && craft_amount < 99) {
                // Step 2: 识别加号按钮后点击 99 次增加组装数量
                increase_craft_amount(*params, 99 - craft_amount);
                sleep(500);

                // Step 3: 识别组装数量
                if (!calc_craft_amount(theme, craft_amount)) {
                    break;
                }

                if (craft_amount < 99) {
                    // 再次点击一次加号按钮, 确认组装数量已为最大
                    int old_craft_amount = craft_amount;
                    ProcessTask(*this, { theme + "@RA@IncreaseCraftAmount" }).run();
                    sleep(500);
                    if (!calc_craft_amount(theme, craft_amount) || craft_amount <= old_craft_amount) {
                        insufficient_materials = true;
                    }
                    else {
                        Log.info("Reclamation Craft", "| craft amount: ", craft_amount);
                    }
                }
            }

            // Step 4: 开始组装或取消组装
            if (craft_amount <= 0) {
                // 点击空白处取消组装
                ProcessTask(*this, { theme + "@RA@CancelCraft" }).run();
            }
            else {
                // 点击开始组装图标, 获得物资, 点击 <点击空白处继续> 文字位置
                ProcessTask(*this, { theme + "@RA@PIS-Craft" }).run();
            }
            sleep(500);
        }
    }
    return true;
}

void asst::ReclamationCraftTaskPlugin::increase_craft_amount(const ReclamationConfig::Params& params, int amount)
{
    LogTraceFunction;

    const auto& theme = params.theme;

    // InputEvent touch_down_event;
    // touch_down_event.type = InputEvent::Type::TOUCH_DOWN;
    // InputEvent touch_up_event;
    // touch_up_event.type = InputEvent::Type::TOUCH_UP;
    Matcher add_button_analyzer;
    add_button_analyzer.set_task_info(theme + "@RA@IncreaseCraftAmount");
    Rect add_button_rect;

    switch (params.increment_mode) {
    case ReclamationConfig::IncrementMode::Click:
        for (int i = 0; !need_exit() && i < amount; ++i) {
            ProcessTask(*this, { theme + "@RA@IncreaseCraftAmount" }).run();
        }
        break;
    // case IncrementMode::FastClick:
    //     for (int i = 0; i < amount; ++i) {
    //         add_button_analyzer.set_image(ctrler()->get_image());
    //         if (add_button_analyzer.analyze()) {
    //             const Rect add_button_rect = add_button_analyzer.get_result().rect;
    //             touch_down_event.point.x = add_button_rect.x + add_button_rect.width / 2;
    //             touch_down_event.point.y = add_button_rect.y + add_button_rect.height / 2;
    //             Log.info(touch_down_event.point.x, touch_down_event.point.y);
    //             ctrler()->inject_input_event(touch_down_event);
    //             ctrler()->inject_input_event(touch_up_event);
    //             sleep(500);
    //         }
    //     }
    //     break;
    case ReclamationConfig::IncrementMode::Hold:
        add_button_analyzer.set_image(ctrler()->get_image());
        if (!add_button_analyzer.analyze()) {
            Log.error(__FUNCTION__, "| cannot recongnise the add button");
            return;
        }
        add_button_rect = add_button_analyzer.get_result().rect;
        ctrler()->swipe(add_button_rect, add_button_rect, 10 * 1000 * amount / 100 + 1000);
        break;
    default:
        Log.error(__FUNCTION__, "| unknown increment mode: ", static_cast<int>(params.increment_mode));
        break;
    }
}

bool asst::ReclamationCraftTaskPlugin::calc_craft_amount(const std::string& theme, int& value)
{
    LogTraceFunction;

    RegionOCRer craft_amount_analyzer(ctrler()->get_image());
    craft_amount_analyzer.set_task_info(theme + "@RA@PIS-CraftAmountOcr");

    std::string value_str;
    if (craft_amount_analyzer.analyze()) {
        value_str = craft_amount_analyzer.get_result().text;
    }
    else { // 此时组装数量显示为灰色的 1
        value_str = "0";
    }

    if (!utils::chars_to_number(value_str, value)) {
        Log.error(__FUNCTION__, "| unable to convert OCR result to integer: ", value_str);
        value = -1;
        return false;
    }

    Log.info(__FUNCTION__, "| craft amount: ", value);
    return true;
}
