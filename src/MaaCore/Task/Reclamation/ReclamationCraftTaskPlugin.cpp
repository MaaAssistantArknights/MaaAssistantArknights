#include "ReclamationCraftTaskPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"

using namespace asst;

bool ReclamationCraftTaskPlugin::load_params(const json::value& params)
{
    LogTraceFunction;

    const ReclamationMode& mode = m_config->get_mode();
    if (mode != ReclamationMode::ProsperityInSave) {
        return false;
    }

    // 根据 params 设置插件专用参数
    m_tool_to_craft = params.get("tool_to_craft", "荧光棒");
    m_num_craft_batches = params.get("num_craft_batches", 16); // 默认值 16 以组装 <荧光棒> 消耗至少 3000 木头为准计算得出

    return true;
}

bool ReclamationCraftTaskPlugin::verify(const AsstMsg msg, const json::value& details) const
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

bool ReclamationCraftTaskPlugin::_run()
{
    LogTraceFunction;

    const std::string& theme = m_config->get_theme();

    // 将要组装的道具设置为 m_tool_to_craft
    Task.get<OcrTaskInfo>(theme + "@RA@PIS-ClickTool")->text = { m_tool_to_craft };

    bool insufficient_materials = false;
    for (int batch = 0; !need_exit() && !insufficient_materials && batch < m_num_craft_batches; ++batch) {
        // Step 1: 选择要组装的道具, 若没有找到则在组装台界面向下滑动屏幕后再次检测, 直到识别到要组装的道具后点击
        ProcessTask(*this, { theme + "@RA@PIS-SelectTool" }).run();
        sleep(100);

        int craft_amount = 0;
        while (!need_exit() && !insufficient_materials && craft_amount < 99 ) {
            // Step 2: 识别加号按钮后点击 99 次增加组装数量
            for (int i = craft_amount; i < 99; ++i) {
                ProcessTask(*this, { theme + "@RA@IncreaseCraftAmount" }).run();
            }
            sleep(100);

            // Step 3: 识别组装数量
            if (!calc_craft_amount(craft_amount)) {
                return false;
            }

            if (craft_amount < 99) {
                // 再次点击一次加号按钮, 确认组装数量已为最大
                int old_craft_amount = craft_amount;
                ProcessTask(*this, { theme + "@RA@IncreaseCraftAmount" }).run();
                sleep(100);
                if (!calc_craft_amount(craft_amount) || craft_amount <= old_craft_amount) {
                    insufficient_materials = true;
                }
                else {
                    Log.info("Reclamation Craft", "| craft amount: ", craft_amount);
                }
            }
        }

        // Step 4: 开始组装或取消组装
        if (craft_amount == 0) {
            // 点击空白处取消组装
            ProcessTask(*this, { theme + "@RA@CancelCraft" }).run();
        } else {
            // 点击开始组装图标, 获得物资, 点击 <点击空白处继续> 文字位置
            ProcessTask(*this, { theme + "@RA@PIS-Craft" }).run();
        }
        sleep(100);
    }

    return true;
}

bool ReclamationCraftTaskPlugin::calc_craft_amount(int& value)
{
    const std::string& theme = m_config->get_theme();

    OCRer craft_amount_analyzer(ctrler()->get_image());
    craft_amount_analyzer.set_task_info(theme + "@RA@PIS-CraftAmountOcr");

    if (!craft_amount_analyzer.analyze()) {
        Log.error(__FUNCTION__, "| unable to detect the craft amount");
        return false;
    }

    const std::string value_str = craft_amount_analyzer.get_result().front().text;
    if (!utils::chars_to_number(value_str, value)) {
        Log.error(__FUNCTION__, "| unable to convert OCR result " + value_str + " to integer");
        return false;
    }

    Log.info(__FUNCTION__, "| craft amount: ", value);
    return true;
}
