#pragma once
#include "Task/Roguelike/AbstractRoguelikeTaskPlugin.h"

#include "Common/AsstTypes.h"
#include "Config/Roguelike/JieGarden/RoguelikeCoppersConfig.h"
#include "Task/Roguelike/JieGarden/RoguelikeCopperStrategy.h"
#include "Vision/Roguelike/JieGarden/RoguelikeCoppersAnalyzer.h"

namespace asst
{

enum class CoppersTaskRunMode
{
    PICKUP,  // 拾取掉落通宝模式
    EXCHANGE // 交换已有通宝模式
};

// 插件执行结果状态
enum class CopperTaskResult
{
    SUCCESS, // 成功
    SKIPPED, // 放弃通宝
    FAILED   // 失败
};

// RoguelikeCoppersTaskPlugin 类实现界园肉鸽通宝的自动拾取和交换逻辑
class RoguelikeCoppersTaskPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeCoppersTaskPlugin() override = default;

    // 加载插件参数，检查是否支持当前主题
    virtual bool load_params(const json::value& params) override;

    // 验证消息是否需要激活插件
    virtual bool verify(AsstMsg msg, const json::value& details) const override;

    // 重置运行时变量，为新一轮执行做准备
    virtual void reset_in_run_variables() override;

protected:
    // 执行插件主要逻辑，根据运行模式处理拾取或交换
    virtual bool _run() override;

private:
    // 处理拾取模式：分析掉落通宝并选择最优的拾取
    CopperTaskResult handle_pickup_mode();

    // 处理交换模式：扫描钱盒通宝并决定是否交换
    CopperTaskResult handle_exchange_mode();

    // 滑动通宝列表的辅助函数
    bool swipe_copper_list(int times, bool to_left) const;
    bool swipe_copper_list_left(int times) const;
    bool swipe_copper_list_right(int times) const;

    bool swipe_copper_list_to_leftmost(int times) const;
    bool swipe_copper_list_to_rightmost(int times) const;

    // 根据行列位置计算并点击通宝
    void click_copper_at_position(int col, int row) const;

    // 辅助函数：根据列类型更新坐标基准点
    void update_column_coordinates(const RoguelikeCoppersAnalyzer::ColumnMetrics& metrics, int col, bool is_last_col);

    // 根据识别到的名称创建通宝对象
    std::optional<RoguelikeCopper> create_copper_from_name(
        const std::string& name, // 通宝名称
        int col = 0,             // 列位置
        int row = 0,             // 行位置
        bool is_cast = false,    // 是否已投出
        const Rect& pos = Rect() // 位置信息，用于调试
    );

    // 调试绘制辅助函数，在图像上绘制检测结果
    void draw_detection_debug(
        cv::Mat& image,
        const RoguelikeCoppersAnalyzer::CopperDetection& detection,
        const cv::Scalar& color) const;

    // 保存调试图像到文件
    void save_debug_image(const cv::Mat& image, const std::string& suffix, bool auto_clean = true) const;

    // 获取当前游戏模式对应的通宝策略类型
    CopperStrategyType get_current_strategy_type() const;

    // 根据策略决定是否交换通宝
    bool decide_exchange_by_strategy(
        const RoguelikeCopper& new_copper,
        const std::vector<RoguelikeCopper>& existing_coppers,
        size_t& discard_index) const;

    mutable asst::CoppersTaskRunMode m_run_mode; // 当前运行模式

    // ———————— 运行时状态变量 ——————————————————————————————————
    std::vector<RoguelikeCopper> m_copper_list;                      // 当前钱盒中的通宝列表
    RoguelikeCopper m_new_copper;                                    // 新拾取的通宝
    std::vector<std::pair<RoguelikeCopper, Point>> m_pending_copper; // 待拾取的通宝及其坐标

    // ———————— 坐标计算相关变量 ——————————————————————————————————
    int m_col = 0;        // 列数
    int m_origin_x = 0;   // 第一列节点的默认横坐标 (Rect.x)
    int m_x = 0;          // 当前屏幕中第一列节点实时识别的横坐标 (Rect.x)
    int m_last_x = 0;     // 最后一列节点的横坐标 (Rect.x)
    int m_y = 0;          // 当前屏幕中第一行节点实时识别的纵坐标 (Rect.y)
    int m_row_offset = 0; // 两行节点之间的距离
};

}
