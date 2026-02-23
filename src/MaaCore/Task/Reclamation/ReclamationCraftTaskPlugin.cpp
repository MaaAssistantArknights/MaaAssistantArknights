#include "ReclamationCraftTaskPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

bool asst::ReclamationCraftTaskPlugin::load_params(const json::value& params)
{
    LogTraceFunction;

    if (const ReclamationMode& mode = m_config->get_mode(); mode != ReclamationMode::ProsperityInSave) {
        return false;
    }

    // 根据 params 设置插件专用参数
    auto tools_to_craft_opt = params.find<json::array>("tools_to_craft");
    for (auto& tool : tools_to_craft_opt.value()) {
        if (std::string tool_str = tool.as_string(); !tool_str.empty()) {
            m_tools_to_craft.emplace_back(tool_str);
        }
    }
    m_num_craft_batches =
        params.get("num_craft_batches", 16); // 默认值 16 以组装 <荧光棒> 消耗至少 3000 木头为准计算得出
    const int increment_mode_int = params.get("increment_mode", static_cast<int>(IncrementMode::Click));
    const auto increment_mode = static_cast<IncrementMode>(increment_mode_int);
    if (increment_mode != IncrementMode::Click && increment_mode != IncrementMode::Hold) {
        Log.error(__FUNCTION__, "| unknown increment mode: ", increment_mode_int);
        return false;
    }
    m_increment_mode = increment_mode;

    return true;
}

bool asst::ReclamationCraftTaskPlugin::verify(const AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    const std::string& task_name = details.get("details", "task", "");
    if (task_name == m_config->get_theme() + "@RA@PIS-CraftTool") {
        return true;
    }

    return false;
}

bool asst::ReclamationCraftTaskPlugin::_run()
{
    LogTraceFunction;

    const std::string& theme = m_config->get_theme();
    for (const std::string& tool_to_craft : m_tools_to_craft) {
        // 将要组装的道具设置为 tool
        Task.get<OcrTaskInfo>(theme + "@RA@PIS-ClickTool")->text = { tool_to_craft };

        bool insufficient_materials = false;
        for (int batch = 0; !need_exit() && !insufficient_materials && batch < m_num_craft_batches; ++batch) {
            // Step 1: 选择要组装的道具, 若没有找到则在组装台界面向下滑动屏幕后再次检测, 直到识别到要组装的道具后点击
            ProcessTask(*this, { theme + "@RA@PIS-SelectTool" }).run();
            sleep(500);

            int craft_amount = 0;
            while (!need_exit() && !insufficient_materials && craft_amount < 99) {
                // Step 2: 识别加号按钮后点击 99 次增加组装数量
                increase_craft_amount(99 - craft_amount);
                sleep(500);

                // Step 3: 识别组装数量
                if (!calc_craft_amount(craft_amount)) {
                    break;
                }

                if (craft_amount < 99) {
                    // 再次点击一次加号按钮, 确认组装数量已为最大
                    int old_craft_amount = craft_amount;
                    ProcessTask(*this, { theme + "@RA@IncreaseCraftAmount" }).run();
                    sleep(500);
                    if (!calc_craft_amount(craft_amount) || craft_amount <= old_craft_amount) {
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
                // 点击开始组装图标, 获得物资, 点击 <获得物资> 文字位置
                ProcessTask(*this, { theme + "@RA@PIS-Craft" }).run();
            }
            sleep(500);
        }
    }
    return true;
}

void asst::ReclamationCraftTaskPlugin::increase_craft_amount(const int& amount)
{
    LogTraceFunction;

    const std::string& theme = m_config->get_theme();

    // InputEvent touch_down_event;
    // touch_down_event.type = InputEvent::Type::TOUCH_DOWN;
    // InputEvent touch_up_event;
    // touch_up_event.type = InputEvent::Type::TOUCH_UP;
    Matcher add_button_analyzer;
    add_button_analyzer.set_task_info(theme + "@RA@IncreaseCraftAmount");
    Rect add_button_rect;

    switch (m_increment_mode) {
    case IncrementMode::Click:
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
    case IncrementMode::Hold:
        add_button_analyzer.set_image(ctrler()->get_image());
        if (!add_button_analyzer.analyze()) {
            Log.error(__FUNCTION__, "| cannot recongnise the add button");
            return;
        }
        add_button_rect = add_button_analyzer.get_result().rect;
        ctrler()->swipe(add_button_rect, add_button_rect, 10 * 1000 * amount / 100 + 1000);
        break;
    default:
        Log.error(__FUNCTION__, "| unknown increment mode: ", static_cast<int>(m_increment_mode));
        break;
    }
}

bool asst::ReclamationCraftTaskPlugin::calc_craft_amount(int& value)
{
    LogTraceFunction;

    const std::string& theme = m_config->get_theme();

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
