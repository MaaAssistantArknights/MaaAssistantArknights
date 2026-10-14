#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Common/AsstTypes.h"

namespace cv
{
class Mat;
}

namespace asst::blackflow
{
struct ScrapTradeInventoryItem
{
    std::string name;
    int row = 0;
    int column = 0;
};

// 顺序为从左到右、从上到下；园圃占据首格，不存入物品列表；未知格以空名称占位。
class ScrapTradeInventoryModel final
{
public:
    void reset(std::vector<std::string> names);

    [[nodiscard]] const std::vector<std::string>& items() const noexcept { return m_items; }

    [[nodiscard]] ScrapTradeInventoryItem item(std::size_t index) const;
    void erase(std::size_t index);

private:
    std::vector<std::string> m_items;
};

struct ScrapTradeSurvey
{
    // 识别出名称的物品；没有识别出名称的零件不在其中。
    std::vector<std::string> items;
    // 位置从顶到底连贯时同名件数准确；画面分段时同名件数取各段最大值，只是下限。
    bool exact_counts = true;
    // 夹在已识别物品之间、没有识别出名称的格子数。
    int unknown_slots = 0;
};

struct ScrapTradeInventoryTarget
{
    TextRect text;
    // 名称查找退路不能确定唯一格子时，不猜测实例编号。
    std::optional<std::size_t> index;
};

class BlackFlowScrapTradeInventoryContext
{
public:
    virtual ~BlackFlowScrapTradeInventoryContext() = default;

    virtual cv::Mat capture() const = 0;
    virtual bool interrupted() const = 0;
    virtual bool precise_swipe_supported() const = 0;
    virtual bool swipe_by(std::string_view task, std::optional<int> distance, std::string* error) = 0;
    virtual bool click(const Rect& rect) = 0;
    virtual bool execute(std::string_view task, std::string* error) = 0;
    virtual std::string last_task() const = 0;
    virtual void wait(unsigned milliseconds) const = 0;
    virtual void on_inventory_item(const ScrapTradeInventoryItem& item) = 0;
};

class BlackFlowScrapTradeInventory final
{
public:
    // 位置确定时逐格回调，之后按位置查找；位置无法确定时之后按名称查找。识别或操作失败时返回空值并写入 error。
    [[nodiscard]] std::optional<ScrapTradeSurvey>
        survey(BlackFlowScrapTradeInventoryContext& context, const std::vector<std::string>& names, std::string* error);

    // 复用位置模型；有歧义后持续按名称查找，新增物品后再尝试重建。
    [[nodiscard]] std::optional<ScrapTradeInventoryTarget> find(
        BlackFlowScrapTradeInventoryContext& context,
        const std::vector<std::string>& names,
        const std::vector<std::string>& wanted,
        std::string* error);

    // 确认流程成功后才删除物品并补位；调用方随后更新交易账本。
    bool
        sell(BlackFlowScrapTradeInventoryContext& context, const ScrapTradeInventoryTarget& target, std::string* error);
    void invalidate();
    void forget_position();

private:
    enum class LocationMode
    {
        Unknown,
        Indexed,
        ByName,
    };

    struct VisibleItem
    {
        TextRect text;
        int column = 0;
        int center_y = 0;
    };

    using View = std::vector<VisibleItem>;

    [[nodiscard]] std::optional<View> recognize(
        BlackFlowScrapTradeInventoryContext& context,
        const std::vector<std::string>& names,
        std::string* error) const;
    [[nodiscard]] std::optional<int> measure_shift(const View& before, const View& after) const;
    [[nodiscard]] bool same_view(const View& before, const View& after) const;
    bool
        advance(BlackFlowScrapTradeInventoryContext& context, const View& view, bool forward, std::string* error) const;
    [[nodiscard]] std::optional<View> rewind(
        BlackFlowScrapTradeInventoryContext& context,
        const std::vector<std::string>& names,
        std::string* error,
        bool* at_top = nullptr) const;
    [[nodiscard]] std::optional<int> resolve_offset(const View& view) const;
    [[nodiscard]] std::optional<TextRect> find_by_name(
        BlackFlowScrapTradeInventoryContext& context,
        const std::vector<std::string>& names,
        const std::vector<std::string>& wanted,
        std::string* error) const;

    ScrapTradeInventoryModel m_model;
    LocationMode m_mode = LocationMode::Unknown;
    std::optional<int> m_offset;
};
} // namespace asst::blackflow
