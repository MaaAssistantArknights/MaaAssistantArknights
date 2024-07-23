#include "RoguelikeConfig.h"

#include "Utils/Logger.hpp"

bool asst::RoguelikeConfig::set_params(const json::value& params)
{
    // ------------------ 肉鸽主题设置 ------------------
    std::string theme = params.get("theme", std::string(RoguelikeTheme::Phantom));
    if (!RoguelikeConfig::is_valid_theme(theme)) {
        Log.error("Unknown roguelike theme", theme);
        return false;
    }

    auto mode = static_cast<RoguelikeMode>(params.get("mode", 0));
    if (!RoguelikeConfig::is_valid_mode(mode, theme)) {
        Log.error(__FUNCTION__, "| Unknown mode", static_cast<int>(mode));
        return false;
    }

    set_theme(theme);
    set_mode(mode);
    set_difficulty(0);

    // 凹指定干员开局直升
    set_start_with_elite_two(params.get("start_with_elite_two", false));
    set_only_start_with_elite_two(params.get("only_start_with_elite_two", false));

    if (mode == RoguelikeMode::Investment) {
        bool investment_with_more_score = params.get("investment_with_more_score", false);
        if (!params.contains("investment_with_more_score") && params.contains("investment_enter_second_floor")) {
            investment_with_more_score = params.get("investment_enter_second_floor", true);
            Log.warn("================  DEPRECATED  ================");
            Log.warn(
                "`investment_enter_second_floor` has been deprecated since v5.2.1; Please use "
                "'investment_with_more_score'");
            Log.warn("================  DEPRECATED  ================");
        }
        set_invest_with_more_score(investment_with_more_score);
    }

    // =========================== 萨米主题专用参数 ===========================

    if (theme == RoguelikeTheme::Sami) {
        // 是否凹开局远见密文板
        set_first_floor_foldartal(params.contains("first_floor_foldartal"));

        // 是否检查坍缩范式，非CLP_PDS模式下默认为False, CLP_PDS模式下默认为True
        set_check_clp_pds(params.get("check_collapsal_paradigms", mode == RoguelikeMode::CLP_PDS));
    }

    return true;
}

void asst::RoguelikeConfig::clear()
{
    // ------------------ 通用参数 ------------------
    m_trader_no_longer_buy = false;
    m_core_char = std::string();
    m_squad = std::string();
    m_team_full_without_rookie = false;
    m_use_support = false;
    m_oper = std::unordered_map<std::string, RoguelikeOper>();

    // ------------------ 萨米主题专用参数 ------------------
    m_foldartal = std::vector<std::string>();
    m_clp_pds = std::vector<std::string>();
    m_need_check_panel = false;

}
