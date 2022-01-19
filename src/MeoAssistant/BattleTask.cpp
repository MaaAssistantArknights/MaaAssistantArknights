#include "BattleTask.h"

#include "BattleImageAnalyzer.h"
#include "BattlePerspectiveImageAnalyzer.h"
#include "Controller.h"
#include "TaskData.h"

typedef asst::BattleImageAnalyzer::Role Role;
typedef asst::BattleImageAnalyzer::Oper Oper;

bool asst::BattleTask::_run()
{
    while (!need_exit()
        && auto_battle()) {
        ;
    }
    return true;
}

bool asst::BattleTask::auto_battle()
{
    BattleImageAnalyzer oper_analyzer(Ctrler.get_image());
    if (!oper_analyzer.analyze()) {
        return false;
    }
    const auto& opers = oper_analyzer.get_opers();
    if (opers.empty()) {
        return true;
    }

    static const std::array<Role, 9> RoleOrder = {
        Role::Pioneer,
        Role::Sniper,
        Role::Support,
        Role::Warrior,
        Role::Caster,
        Role::Special,
        Role::Medic,
        Role::Tank,
        Role::Drone
    };
    const auto use_oper_task_ptr = Task.get("BattleUseOper");

    // 点击当前最合适的干员

    //auto oper_iter = std::find_first_of(
    //    opers.cbegin(), opers.cend(), RoleOrder.cbegin(), RoleOrder.cend(),
    //    [](const Oper& oper, const Role& role) -> bool {
    //        return oper.available && oper.role == role;
    //});
    //if (oper_iter == opers.cend()) {
    //    return true;
    //}
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
    placed_analyzer.set_src_homes(oper_analyzer.get_homes());
    if (!placed_analyzer.analyze()) {
        return true;
    }
    Point nearest_point = placed_analyzer.get_nearest_point();
    Rect nearest_rect(nearest_point.x, nearest_point.y, 1, 1);
    Ctrler.swipe(opt_oper.rect, nearest_rect, 1000);
    sleep(use_oper_task_ptr->rear_delay);

    // 拖动干员朝向
    Rect home = placed_analyzer.get_homes().front();
    Point home_center(home.x + home.width / 2, home.y + home.height / 2);

    switch (opt_oper.role) {
    case Role::Medic:
    case Role::Support:
        Ctrler.swipe(nearest_point, home_center, 300);
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
        Point reverse_point;
        reverse_point.x = nearest_point.x * 2 - home_center.x;
        reverse_point.y = nearest_point.y * 2 - home_center.y;
        Ctrler.swipe(nearest_point, reverse_point, 300);
    }
    break;
    }
    return true;
}
