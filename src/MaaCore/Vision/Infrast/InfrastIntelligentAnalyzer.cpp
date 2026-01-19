#include "InfrastIntelligentAnalyzer.h"

#include "Config/TaskData.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"

#include "Utils/NoWarningCV.h"

// 硬编码的坐标参数，模仿你的需求
namespace
{
    const int ROOM_NAME_X = 430;
    const int ROOM_NAME_W = 187;
    const int ROOM_HEIGHT = 135;
    const std::vector<int> SLOT_X_OFFSETS = { 624, 736, 847, 958, 1073 };
    const int SLOT_WIDTH_AVG = 114;
    const int SLOT_HEIGHT_AVG = 134;
}

bool asst::InfrastIntelligentAnalyzer::analyze()
{
    LogTraceFunction;
    m_result.clear(); // 每次分析前清空上次结果

    // --- 1. 使用 MultiMatcher 寻找所有房间锚点 ---
    MultiMatcher anchor_matcher(m_image);
    anchor_matcher.set_task_info("InfrastOverview-Anchor");

    if (!anchor_matcher.analyze()) {
        Log.info("InfrastIntelligentAnalyzer | No anchors found");
        return false;
    }

    // 获取房间名配置 (虽然后面暂时只画框，不识别)
    const auto& name_task_ptr = Task.get("InfrastOverview-NameRect");
    // 获取空槽位判断配置
    const auto& empty_check_task_ptr = Task.get("InfrastOverview-SlotEmpty");
    
    auto anchors = anchor_matcher.get_result();
    
    // 排序：按 Y 坐标从小到大（从上到下）
    std::sort(anchors.begin(), anchors.end(), [](const MatchRect& a, const MatchRect& b) {
        return a.rect.y < b.rect.y;
    });

    // --- 2. 遍历锚点，构建每一行的数据 ---
    for (const auto& match : anchors) {
        const auto& anchor_rect = match.rect;
        
#ifdef ASST_DEBUG
        // 绿色：锚点
        cv::rectangle(m_image_draw, make_rect<cv::Rect>(anchor_rect), cv::Scalar(0, 255, 0), 2);
#endif

        infrast::InfrastRoomInfo room_info;
        room_info.anchor_rect = anchor_rect; // 记录锚点位置（调试用）
        // 2.1 确定房间名区域
        if (name_task_ptr) {
            // move函数逻辑：new_x = anchor.x + task.roi.x, new_y = anchor.y + task.roi.y ...
            // 注意：MAA的Rect::move实现可能根据版本有所不同，但通常用于相对偏移
            room_info.name_rect = anchor_rect.move(name_task_ptr->roi);

#ifdef ASST_DEBUG
            // 蓝色：房间名区域
            cv::rectangle(m_image_draw, make_rect<cv::Rect>(room_info.name_rect), cv::Scalar(255, 0, 0), 2);
#endif
        }

        // 2.2 遍历 5 个槽位
        for (size_t i = 0; i < SLOT_X_OFFSETS.size(); ++i) {
            std::string task_name = "InfrastOverview-Slot-" + std::to_string(i);
            const auto& slot_task_ptr = Task.get(task_name);

            if (!slot_task_ptr) {
                Log.warn("Missing task config:", task_name);
                continue;
            }
            // 计算该槽位的绝对 ROI
            auto slot_roi = anchor_rect.move(slot_task_ptr->roi);

            // 边界检查，防止越界
            if ((slot_roi.x + slot_roi.width > m_image.cols) ||
                (slot_roi.y + slot_roi.height > m_image.rows)) {
                room_info.slots_rect.emplace_back(); // 存个空的占位
                room_info.slots_empty.push_back(false);
                continue;
            }

            room_info.slots_rect.push_back(slot_roi);

            // 识别是否为空位
            bool is_empty = false;
            if (empty_check_task_ptr) {
                Matcher empty_matcher(m_image);
                empty_matcher.set_task_info(empty_check_task_ptr);
                empty_matcher.set_roi(slot_roi); // 限定在槽位区域内找
                
                if (empty_matcher.analyze()) {
                    is_empty = true;
                }
            }
            room_info.slots_empty.push_back(is_empty);

#ifdef ASST_DEBUG
            // 黄色：空位；青色：有人
            cv::Scalar color = is_empty ? cv::Scalar(0, 255, 255) : cv::Scalar(255, 255, 0);
            cv::rectangle(m_image_draw, make_rect<cv::Rect>(slot_roi), color, 2);
#endif
        }
        
        // 将这一行的数据存入结果集
        m_result.emplace_back(std::move(room_info));
    }

    return !m_result.empty();
}