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
#include "Vision/RegionOCRer.h"

bool asst::RoguelikeShoppingTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (!RoguelikeConfig::is_valid_theme(m_config->get_theme())) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }

    return details.get("details", "task", "").ends_with("Roguelike@TraderRandomShopping");
}

bool asst::RoguelikeShoppingTaskPlugin::_run()
{
    if (m_config->get_investment_enabled()) investing();

    if (m_config->get_mode() != RoguelikeMode::Investment) {
        return shopping();
    }

    if (m_config->get_mode() == RoguelikeMode::Investment && !m_config->get_investment_enter_second_floor()) {
        exit_to_home_page();
    }

    return true;
}

void asst::RoguelikeShoppingTaskPlugin::investing()
{
    LogTraceFunction;

    auto ocr_current_count = [](const auto& img, const auto& task_name) -> std::optional<int> {
        RegionOCRer ocr(img);
        ocr.set_task_info(task_name);
        if (!ocr.analyze()) {
            Log.error(__FUNCTION__, "unable to analyze current investment count.");
            return std::nullopt;
        };
        int count = 0;
        if (!utils::chars_to_number(ocr.get_result().text, count)) {
            Log.error(__FUNCTION__, "unable to convert current investment count<", ocr.get_result().text,
                      "> to number.");
            return std::nullopt;
        }
        return count;
    };
    auto find_confirm_rect = [&](const auto& img) -> std::optional<asst::Rect> {
        auto task = ProcessTask(
            *this, { "Roguelike@StageTraderInvestSystemError", "Roguelike@StageTraderInvestSystemInvalid" });
        task.set_reusable_image(img).set_retry_times(0);
        if (task.run()) {
            return std::nullopt;
        }
        Matcher match(img);
        match.set_task_info("Roguelike@StageTraderInvestConfirm");
        if (!match.analyze()) {
            Log.error(__FUNCTION__, "unable to find invest click rect.");
            return std::nullopt;
        }
        return match.get_result().rect;
    };
    auto leave_invest = [&]() {
        ProcessTask(*this, { "Roguelike@StageTraderInvestCancel", "Roguelike@StageTraderInvestSystemLeave" })
            .set_retry_times(0)
            .run();
    };

    // 可用投资次数
    int count_limit = m_config->get_investment_count_limit() - m_config->get_investment_count();

    auto img = ctrler()->get_image();
    // 当前的存款
    auto deposit = ocr_current_count(img, "Roguelike@StageTrader-InvestCount");

    auto enter_task =
        ProcessTask(*this, { "Roguelike@StageTraderInvestSystemEnter", "Roguelike@StageTraderInvestSystem" });
    if (!enter_task.set_reusable_image(img).run()) {
        // 进入投资页面失败
        Log.error(__FUNCTION__, "unable to enter invest system.");
        leave_invest();
        return;
    }

    // 进入投资页面成功
    // 999 >> 1000
    // 取消 | 确认投资

    Matcher match(ctrler()->get_image());
    match.set_task_info("Roguelike@StageTraderInvestConfirm");
    if (!match.analyze()) {
        Log.error(__FUNCTION__, "unable to find invest click rect.");
        leave_invest();
        return;
    }
    const auto& click_rect = match.get_result().rect;

    int count = 0; // 已投资的个数
    int retry = 0; // 重试次数
    while (!need_exit() && deposit && *deposit < 999 && count_limit > 0 && retry < 3) {
        int times = std::min(20, count_limit - count);
        while (!need_exit() && times > 0) {
            ctrler()->click(click_rect);
            times--;
            sleep(100);
        }
        img = ctrler()->get_image();
        // 检查是否处于可投资状态
        if (!find_confirm_rect(img)) {
            // 投资系统错误 / 没钱了
            break;
        }
        else if (auto ocr = ocr_current_count(img, "Roguelike@StageTraderInvest-Count"); ocr) {
            // 可继续投资 / 到达投资上限999
            if (*ocr == *deposit || *ocr > 999 || *ocr < 0) {
                retry++; // 可能是出错了，重试三次放弃
            }
            else {
                count += *ocr - *deposit;
                deposit = *ocr;
                retry = 0;
            }
        }
        else {
            Log.error(__FUNCTION__, "未知状态");
            leave_invest();
            return;
        }
    }
    leave_invest();
    if (auto ocr = ocr_current_count(ctrler()->get_image(), "Roguelike@StageTrader-InvestCount"); ocr) {
        // 可继续投资 / 到达投资上限999
        count += *ocr - deposit.value_or(0);
        deposit = *ocr;
    }

    auto total = count + m_config->get_investment_count();

    auto info = basic_info_with_what("RoguelikeInvestment");
    info["details"]["count"] = count;
    info["details"]["total"] = total;
    info["details"]["deposit"] = deposit ? *deposit : -1;
    callback(AsstMsg::SubTaskExtraInfo, info);

    Log.info(__FUNCTION__, "本轮投资结束, 投资", count, ", 共投资", total, "; 系统余额", deposit ? *deposit : -1);
    m_config->set_investment_count(total);

    if (count_limit - count <= 0) {
        Log.info(__FUNCTION__, "投资达到设置上限,", m_config->get_investment_count_limit());
        exit_to_home_page();
        m_task_ptr->set_enable(false);
        return;
    }

    if (deposit.value_or(0) == 999 && m_config->get_investment_stop_when_full()) {
        Log.info(__FUNCTION__, "存款已满");
        exit_to_home_page();
        m_task_ptr->set_enable(false);
    }

    return;
}

bool asst::RoguelikeShoppingTaskPlugin::shopping()
{
    LogTraceFunction;

    OCRer analyzer;
    analyzer.set_task_info("RoguelikeTraderShoppingOcr");
    auto image = ctrler()->get_image();
    analyzer.set_image(image);
    if (!analyzer.analyze()) {
        return false;
    }

    bool no_longer_buy = m_config->get_trader_no_longer_buy();

    std::unordered_map<battle::Role, size_t> map_roles_count;
    std::unordered_map<battle::Role, size_t> map_wait_promotion;
    size_t total_wait_promotion = 0;
    std::unordered_set<std::string> opers_list;
    for (const auto& [name, oper] : m_config->get_oper()) {
        int elite = oper.elite;
        int level = oper.level;
        Log.info(name, elite, level);

        // 等级太低的干员没必要为他专门买收藏品什么的
        if (level < 60) {
            continue;
        }

        opers_list.emplace(name);

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

    auto& all_goods = RoguelikeShopping.get_goods(m_config->get_theme());
    std::vector<std::string> all_foldartal = m_config->get_theme() == RoguelikeTheme::Sami
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
            if (ranges::find_first_of(opers_list, goods.chars) == opers_list.cend()) {
                Log.trace("Ready to buy", goods.name, ", but there is no such character, skip");
                continue;
            }
        }

        Log.info("Ready to buy", goods.name);
        ctrler()->click(find_it->rect);
        if (m_config->get_theme() == RoguelikeTheme::Sami) {

            auto iter = std::find(all_foldartal.begin(), all_foldartal.end(), goods.name);
            if (iter != all_foldartal.end()) {
                auto foldartal = m_config->get_foldartal();
                // 把goods.name存到密文板overview里
                foldartal.emplace_back(goods.name);
                m_config->set_foldartal(std::move(foldartal));
            }
        }
        if (goods.no_longer_buy) {
            m_config->set_trader_no_longer_buy(true);
        }
        break;
    }

    return true;
}

void asst::RoguelikeShoppingTaskPlugin::exit_to_home_page()
{
    ProcessTask(*this, { m_config->get_theme() + "@Roguelike@ExitThenAbandon" })
        .set_times_limit("Roguelike@StartExplore", 0)
        // .set_times_limit("Roguelike@Abandon", 0)
        .run();
}
