#pragma once

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Config/AbstractConfig.h"

namespace asst::blackflow
{
enum class ScrapCategory
{
    Processing,
    Natural,
    Concept,
};

struct ScrapItem
{
    std::string name;
    ScrapCategory category = ScrapCategory::Processing;
    // 不在行商货架上出售的零件没有购入价。
    std::optional<int> price;
    int value = 0;
    // 持有时每获得一个零件，自身估价的增长量。
    int growth_per_acquisition = 0;
    // 获得时随之获得的零件，例如多生苔藓附带三个枯苔藓球。
    std::vector<std::pair<std::string, int>> bundle;
    // 加工品覆盖的常用移动形状。用途购买只在填补缺少的形状时买入；不填的加工品不受此限制。
    std::vector<std::string> covers;
};

struct ScrapShopLayout
{
    // 按加工品、自然物、概念体的顺序记录一页六格的类别件数。
    std::array<int, 3> counts {};
    double probability = 0;
};

struct ScrapShopType
{
    std::string id;
    std::vector<ScrapShopLayout> layouts;
    // 每页出现率，来自画面统计，属于近似值。
    std::unordered_map<std::string, double> offer_rates;
};
} // namespace asst::blackflow

namespace asst
{
class BlackFlowScrapMarketConfig final :
    public MAA_NS::SingletonHolder<BlackFlowScrapMarketConfig>,
    public AbstractConfig
{
public:
    [[nodiscard]] bool available() const noexcept { return !m_items.empty(); }

    [[nodiscard]] const std::vector<blackflow::ScrapItem>& items() const noexcept { return m_items; }

    [[nodiscard]] std::optional<std::reference_wrapper<const blackflow::ScrapItem>> find(const std::string& name) const;

    [[nodiscard]] const std::vector<int>& refresh_costs() const noexcept { return m_refresh_costs; }

    // 用途购买后持有的加工品不超过此数。
    [[nodiscard]] int processing_keep_limit() const noexcept { return m_processing_keep_limit; }

    [[nodiscard]] const std::vector<blackflow::ScrapShopType>& shop_types() const noexcept { return m_shop_types; }

protected:
    bool parse(const json::value& value) override;

private:
    std::vector<blackflow::ScrapItem> m_items;
    std::unordered_map<std::string, std::size_t> m_index;
    std::vector<int> m_refresh_costs;
    int m_processing_keep_limit = 0;
    std::vector<blackflow::ScrapShopType> m_shop_types;
};

inline auto& BlackFlowScrapMarket = BlackFlowScrapMarketConfig::get_instance();
} // namespace asst
