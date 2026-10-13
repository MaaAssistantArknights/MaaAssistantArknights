#pragma once

#include <string>
#include <vector>

#include "Common/AsstBattleDef.h"
#include "Common/AsstTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"

namespace asst::oper_progress
{
// 快速编队卡片识别结果，参照 BattleFormationTask::QuickFormationOper 裁剪出选人所需字段。
// 干员培养与基建训练室的自定义排班共用受训干员面板（与作战快速编队同一套 UI）。
struct QuickFormationOperInfo
{
    std::string name;
    asst::Rect flag_rect;
    bool selected = false;
};

// 参照 BattleFormationTask::analyzer_opers：以职业旗标模板定位卡片，
// 对旗标下方区域 OCR 干员名，并以旗标上方的高亮色块判断选中态。
std::vector<QuickFormationOperInfo> analyze_formation_opers(const cv::Mat& image);

// 快速编队界面按职业筛选的 tab 任务名，Unknown 表示 ｢全部｣ 。
std::string role_task_name(asst::battle::Role role);
}
