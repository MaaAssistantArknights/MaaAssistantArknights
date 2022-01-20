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
    if (auto cur_home = oper_analyzer.get_homes();
    !cur_home.empty()) {
        m_home_cache = cur_home;
    }

    static const std::array<Role, 9> RoleOrder = {
        Role::Medic,
        Role::Pioneer,
        Role::Sniper,
        Role::Warrior,
        Role::Support,
        Role::Caster,
        Role::Special,
        Role::Tank,
        Role::Drone
    };
    const auto use_oper_task_ptr = Task.get("BattleUseOper");

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
    Ctrler.swipe(opt_oper.rect, nearest_rect, 1000);
    sleep(use_oper_task_ptr->rear_delay);

    // 拖动干员朝向
    Rect home = placed_analyzer.get_homes().front();
    Point home_center(home.x + home.width / 2, home.y + home.height / 2);

    int dx = nearest_point.x - home_center.x;
    int dy = nearest_point.y - home_center.y;
    Point end_point;

    switch (opt_oper.role) {
    case Role::Medic:
    case Role::Support:
    {
        constexpr int coeff = 5;
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
        constexpr int coeff = 5;
        end_point.x = nearest_point.x + coeff * dx;
        end_point.y = nearest_point.y + coeff * dy;
    }
    break;
    }
    Ctrler.swipe(nearest_point, end_point, 300);
    return true;
}
