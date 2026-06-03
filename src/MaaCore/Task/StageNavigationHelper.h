#pragma once

#include "Utils/Logger.hpp"
#include "Utils/WorkingDir.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

namespace asst::StageNavigationHelper
{
/// <summary>
/// 检查活动关卡是否存在对应的模板截图。
/// 从关卡名中提取活动前缀（如 "TD-6" -> "TD"），再拼接路径
/// resource/template/StageNavigation/SideStory/{PREFIX}/{stage_name}.png
/// </summary>
/// <param name="stage_name">关卡名，如 "TD-6"、"SN-10"</param>
/// <returns>模板相对路径（不含扩展名），如 "StageNavigation/SideStory/TD/TD-6"；不存在则返回空字符串</returns>
inline std::string get_stage_template_path(const std::string& stage_name)
{
    // 从关卡名中提取活动前缀，例如 "TD-6" -> "TD"，"SN-10" -> "SN"
    // 匹配格式：2~3个字母前缀 + "-" + 数字
    auto dash_pos = stage_name.find('-');
    if (dash_pos == std::string::npos || dash_pos < 2 || dash_pos > 3) {
        return {};
    }

    std::string prefix = stage_name.substr(0, dash_pos);
    // 验证前缀全是字母
    if (!std::all_of(prefix.begin(), prefix.end(), [](char c) { return std::isalpha(static_cast<unsigned char>(c)); })) {
        return {};
    }

    // 构造模板相对路径：StageNavigation/SideStory/{PREFIX}/{STAGE_NAME}
    std::string templ_rel_path = "StageNavigation/SideStory/" + prefix + "/" + stage_name;
    std::filesystem::path templ_file = ResDir.get() / "template" / (templ_rel_path + ".png");

    if (std::filesystem::exists(templ_file)) {
        Log.info("Stage template found:", templ_rel_path);
        return templ_rel_path;
    }

    return {};
}
} // namespace asst::StageNavigationHelper
