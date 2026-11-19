#pragma once
#include "Common/AsstTypes.h"
#include "Controller/ControllerAPI.h"
#include "Task/AbstractTaskPlugin.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace asst
{
// 卫戍协议极速模拟刷角标插件：活动主界面经 @Nav 链导航到策略选择页（不复用 MiniGame@SPA 挂机链），
// 按角标选策略，回合准备（按权重购买、升级商店、固定格位部署）、结算判定与循环，直到目标通关局数。
// 界面状态判定走模板匹配（复用挂机链模板），OCR 仅用于小范围数据读取。
class StrongholdProtocolTaskPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~StrongholdProtocolTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

    StrongholdProtocolTaskPlugin& set_target(int target) noexcept;
    StrongholdProtocolTaskPlugin& set_runs(int runs) noexcept;
    StrongholdProtocolTaskPlugin& set_attempts(int attempts) noexcept;

protected:
    virtual bool _run() override;

private:
    enum class Scene
    {
        Navigate,       // 活动主界面（极速模拟入口），走 @Nav 链导航到策略选择页
        StrategySelect, // 策略网格页（歌利亚环节，@Nav 链停在此页）
        ConfirmInfo,    // 确认本局信息页（与编队页同款准备就绪按钮，按页面文字区分）
        CustomSquad,    // 自选编队页（CustomOpersConfirm 模板命中；选人/确认同一场景）
        BountySelect,   // 悬赏决策页（三选一应征目标）
        RoundPrepare,   // 回合休整（商店与准备就绪）
        ConfirmPopup,   // 确认准备吗（有剩余资金/指令时的进入战斗确认弹窗）
        Settlement,     // 结算（返回主页）
        Unknown,
    };

    // 商店单张卡牌（解析自卡牌区 OCR 与局部色判）
    struct ShopCard
    {
        int x = 0;            // 卡牌中心 x（720p 基准）
        int cost = -1;        // 购买费用，-1 未知
        int tier = 0;         // 等阶（罗马数字），0 未知
        bool empty = false;   // 空位
        bool is_oper = false; // 是干员（非装备）
        bool is_dps = false;  // 输出型干员
        std::string name;     // 干员名（可能为空）
    };

    struct PurchaseCount
    {
        int total = 0;
        int unmerged = 0; // 已买入且未确认合成或卖出的份数
    };

    // 每局对局状态，结算后重置
    struct BoardState
    {
        int round_done = -1;         // 已处理完的回合号
        bool pending_deploy = false; // 备战席上还有干员待部署
        bool squad_picked = false;   // 本局已在编队页选过人
        // 已知身份但仍在备战席的干员 (名, 等阶)：买入后没能当场部署、或被换下没卖掉，回合末重扫时据此恢复等阶
        std::vector<std::pair<std::string, int>> bench_known;
        int healers = 0;                                          // 已购买的医疗干员数（前期最多 1 个）
        std::unordered_map<std::string, PurchaseCount> purchases; // 本局各干员的累计购买次数，含免费领取
        std::set<std::string> promotion_tiles;                    // 合成素材曾占用、尚待核对的格位
        std::map<std::string, std::pair<std::string, int>> occ;   // 格位键（组名+序号）-> (干员名, 等阶)
    };

    // 未识别场景时的全屏文字扫描结果
    struct UnknownScreen
    {
        bool in_garrison = false;               // 画面上有卫戍协议界面关键词
        bool in_battle = false;                 // 战斗回合中（含领袖回合），不做通用点击
        std::optional<TextRect> generic_button; // 可点的通用按钮（继续/确认/领取等）
    };

    Scene detect_scene(const cv::Mat& image, UnknownScreen& screen);
    static UnknownScreen scan_unknown_screen(const std::vector<TextRect>& text);
    static bool is_bounty_select(const std::vector<TextRect>& text);
    // 悬赏决策：默认选中间一项，避免个别目标奖励/难度失衡
    bool pick_bounty();
    // 模板命中后直接点击其矩形；不借 ProcessTask，避免任务 next 链把流程带走
    bool click_template(const std::string& task_name);
    enum class StrategyResult
    {
        Selected,
        AllCompleted,
        IncompleteScan,
    };
    StrategyResult pick_strategy();
    bool pick_squad(BoardState& board);
    bool prep_phase(BoardState& board);
    bool handle_settlement(BoardState& board);

    // 商店读取与操作
    struct ShopInfo
    {
        std::optional<int> gold;
        std::optional<int> upgrade_cost; // 升级费用，满级时无效
        bool maxed = false;              // 商店已满级
        std::optional<int> round;        // 当前回合号
        std::optional<int> upgrade_x;    // 升级/满级按钮文字中心 x（按钮随商店等级移动）
    };

    // roi 给出时覆盖任务自身的 roi
    std::optional<int> read_number(
        const cv::Mat& image,
        const std::string& task_name,
        const std::optional<Rect>& roi = std::nullopt) const;
    // 资金读取：失败时换新截图重试并用加宽识别框兜底，仍失败返回 nullopt
    std::optional<int> read_gold(const cv::Mat& image);
    ShopInfo read_shop(const cv::Mat& image);
    std::vector<ShopCard> read_shop_cards(const cv::Mat& image) const;
    static std::optional<ShopCard> promotion_reward(const std::vector<ShopCard>& cards);
    bool buy_oper(ShopCard& card, const cv::Mat& before, cv::Mat& after, std::vector<ShopCard>& cards_after);
    bool buy_and_deploy(BoardState& board, ShopCard card);

    // 触控原语：游戏要求先在干员头像上 press-and-hold 进入拖拽预览再滑动，
    // swipe 快速位移来不及触发 grab；用 inject_input_event 手动 down→hold→move→hold→up
    void emit_input(InputEvent::Type type, const Point& p, long ms = 0);
    void drag_to(const Point& from, const Point& to, int hold_ms, int move_ms);
    // 部署（固定格位布局）：拿起复核（面板名字/等阶/装备）→ lift 悬停 → 固定格表选格 →
    // 拖放 → 朝向确认 → 换人时卖掉顶下的低星、高星标记待重排
    std::optional<Point> deploy_from_bench(
        BoardState& board,
        const Point& bench,
        const std::string& name_hint = "",
        int tier_hint = 0,
        const cv::Mat& before_purchase = {});
    void deploy_bench(BoardState& board);
    bool set_facing(const Point& tile);
    void handle_popups();

    std::optional<int> slots_left(const cv::Mat& image) const;
    std::optional<Point> new_bench_slot(const cv::Mat& before, const cv::Mat& after) const;
    // 备战席小人定位：按头顶深色等阶方牌检测，按压点为方牌左上角加 Roi@BenchPressOffset 偏移
    static std::vector<Point> detect_bench_opers(const cv::Mat& image);

    // 优先级干员判定（权重表在 stronghold_protocol 资源目录的 purchase.json）
    static int oper_priority(const std::string& name);
    static bool is_safe_oper(const std::string& name);
    static bool is_healer(const std::string& name);
    static bool is_dps_oper(const std::string& name);
    static int oper_rank(const std::string& name, int tier);
    static bool name_match(const std::string& name, const std::string& key);

    // 策略网格角标颜色判定（金色=已通关 3 次，彩色=镀层 6 次）
    static bool has_gold_badge(const cv::Mat& image, int cx, int cy);
    static bool has_color_badge(const cv::Mat& image, int cx, int cy);
    static double
        color_ratio(const cv::Mat& image, const Rect& roi, int lower_h, int upper_h, int lower_s, int lower_v);
    // 拖拽预览中格心邻域绿占比 > 0.8 即可部署的空格
    static bool is_green_tile(const cv::Mat& image, const Point& pt, int radius = 15);
    static bool has_chinese(const std::string& text);
    // 策略列表滚动位移测量（模板匹配；位移过小视为已到底）
    static std::optional<int> measure_grid_shift(const cv::Mat& before, const cv::Mat& after);

    int m_target = 3;   // 3 = 金色角标，6 = 彩色角标
    int m_runs = 20;    // 目标通关局数
    int m_attempts = 0; // 总局数上限（胜负都算），0 = 不限
    int m_cleared = 0;
    int m_attempted = 0;
    std::string m_current_strategy;
    std::optional<Point> m_last_tile;  // 最近部署的格子，朝向设置卡住时补设用
    bool m_settlement_counted = false; // 当前结算页已计数，离开结算页后复位
    std::optional<int> m_upgrade_x;    // 最近一次读到的升级按钮中心 x，商店卡牌识别据此剔除按钮区
};
}
