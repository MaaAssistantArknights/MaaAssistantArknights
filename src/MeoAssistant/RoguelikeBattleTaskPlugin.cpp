#include "RoguelikeBattleTaskPlugin.h"

#include "BattleImageAnalyzer.h"
#include "BattlePerspectiveImageAnalyzer.h"
#include "Controller.h"
#include "TaskData.h"
#include "ProcessTask.h"

bool asst::RoguelikeBattleTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted
        || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (details.at("details").at("task").as_string() == "Roguelike1StartAction") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeBattleTaskPlugin::_run()
{
    m_used_opers = false;
    m_pre_hp = 0;
    m_home_cache.clear();

    speed_up();

    while (!need_exit()) {
        // 不在战斗场景，且已使用过了干员，说明已经打完了，就结束循环
        if (!auto_battle() && m_used_opers) {
            break;
        }
    }

    return true;
}

bool asst::RoguelikeBattleTaskPlugin::auto_battle()
{
    using Role = asst::BattleImageAnalyzer::Role;
    using Oper = asst::BattleImageAnalyzer::Oper;

    BattleImageAnalyzer battle_analyzer(Ctrler.get_image());
    if (!battle_analyzer.analyze()) {
        return false;
    }

    if (int hp = battle_analyzer.get_hp();
        hp != 0) {
        bool used_skills = false;
        if (hp < m_pre_hp) {    // 说明漏怪了，漏怪就开技能（
            for (const Rect& rect : battle_analyzer.get_ready_skills()) {
                use_skill(rect);
                used_skills = true;
            }
        }
        m_pre_hp = hp;
        if (used_skills) {
            return true;
        }
    }

    const auto& opers = battle_analyzer.get_opers();
    if (opers.empty()) {
        return true;
    }
    if (auto cur_home = battle_analyzer.get_homes();
        !cur_home.empty()) {
        m_home_cache = cur_home;
    }
    static const std::array<Role, 9> RoleOrder = {
        Role::Pioneer,
        Role::Sniper,
        Role::Warrior,
        Role::Support,
        Role::Medic,
        Role::Caster,
        Role::Special,
        Role::Tank,
        Role::Drone
    };
    const auto use_oper_task_ptr = Task.get("BattleUseOper");
    const auto swipe_oper_task_ptr = Task.get("BattleSwipeOper");

    // 点击当前最合适的干员
    Oper opt_oper;
    bool oper_found = false;
    for (auto role : RoleOrder) {
        for (const auto& oper : opers) {
            if (!oper.available) {
                continue;
            }
            if (oper.role == role) {
                opt_oper = oper;
                oper_found = true;
                break;
            }
        }
        if (oper_found) {
            break;
        }
    }

    Ctrler.click(opt_oper.rect);
    sleep(use_oper_task_ptr->pre_delay);

    // 将干员拖动到场上
    BattlePerspectiveImageAnalyzer placed_analyzer(Ctrler.get_image());
    placed_analyzer.set_src_homes(m_home_cache);
    if (!placed_analyzer.analyze()) {
        return true;
    }
    Point nearest_point = placed_analyzer.get_nearest_point();
    Rect nearest_rect(nearest_point.x, nearest_point.y, 1, 1);
    Ctrler.swipe(opt_oper.rect, nearest_rect, swipe_oper_task_ptr->pre_delay);
    sleep(use_oper_task_ptr->rear_delay);

    // 拖动干员朝向
    Rect home = placed_analyzer.get_homes().front();
    Point home_center(home.x + home.width / 2, home.y + home.height / 2);

    int dx = nearest_point.x - home_center.x;
    int dy = nearest_point.y - home_center.y;
    if ((dx * 7) < (dy * 11)) {
        dx = 0;
    }
    else {
        dy = 0;
    }
    constexpr int coeff = 50;
    Point end_point;
    switch (opt_oper.role) {
    case Role::Medic:
    case Role::Support:
    {
        end_point.x = nearest_point.x - coeff * dx;
        end_point.y = nearest_point.y - coeff * dy;
    }
    break;
    case Role::Pioneer:
    case Role::Warrior:
    case Role::Sniper:
    case Role::Special:
    case Role::Tank:
    case Role::Caster:
    case Role::Drone:
    default:
    {
        end_point.x = nearest_point.x + coeff * dx;
        end_point.y = nearest_point.y + coeff * dy;
    }
    break;
    }
    if (end_point.x < 0) {
        end_point.x = 0;
    }
    else if (end_point.x >= WindowWidthDefault) {
        end_point.x = WindowWidthDefault - 1;
    }
    if (end_point.y < 0) {
        end_point.y = 0;
    }
    else if (end_point.y >= WindowHeightDefault) {
        end_point.y = WindowHeightDefault - 1;
    }
    Ctrler.swipe(nearest_point, end_point, swipe_oper_task_ptr->rear_delay);

    m_used_opers = true;

    return true;
}

bool asst::RoguelikeBattleTaskPlugin::speed_up()
{
    ProcessTask task(*this, { "BattleSpeedUp" });
    return task.run();
}

bool asst::RoguelikeBattleTaskPlugin::use_skill(const asst::Rect& rect)
{
    Ctrler.click(rect);

    ProcessTask task(*this, { "BattleUseSkill" });
    return task.run();
}
