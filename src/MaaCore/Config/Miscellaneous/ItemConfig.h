#pragma once

#include "Config/AbstractConfigWithTempl.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace asst
{
    class ItemConfig final : public SingletonHolder<ItemConfig>, public AbstractConfigWithTempl
    {
    public:
        virtual ~ItemConfig() override = default;

        const std::string& get_item_name(const std::string& id) const noexcept
        {
            if (id.empty()) {
                static const std::string unknown = "Unknown";
                return unknown;
            }
            if (auto iter = m_item_name.find(id); iter != m_item_name.cend()) {
                return iter->second;
            }
            else {
                static const std::string empty;
                return empty;
            }
        }
        const auto& get_all_item_id() const noexcept { return m_all_item_id; }
        virtual const std::unordered_set<std::string>& get_templ_required() const noexcept override
        {
            return get_all_item_id();
        }
        const auto& get_ordered_material_item_id() const noexcept { return m_ordered_material_item_id; }

    protected:
        virtual bool parse(const json::value& json) override;
        void clear();

        // key：材料编号Id，value：材料名（对应客户端材料名称，utf8）
        std::unordered_map<std::string, std::string> m_item_name;
        std::unordered_set<std::string> m_all_item_id;
        std::vector<std::string> m_ordered_material_item_id;
    };

    inline static auto& ItemData = ItemConfig::get_instance();
}
