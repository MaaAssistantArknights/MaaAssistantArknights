#pragma once

#include "Config/AbstractConfigWithTempl.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace asst
{
    enum ItemClassifyType
    {
        None,
        Normal,
        Material,
        Consume,
        Invalid
    };

    inline std::string enum_to_string(ItemClassifyType type)
    {
        static const std::unordered_map<ItemClassifyType, std::string> map = { { ItemClassifyType::None, "NONE" },
                                                                               { ItemClassifyType::Normal, "NORMAL" },
                                                                               { ItemClassifyType::Material, "MATERIAL" },
                                                                               { ItemClassifyType::Consume, "CONSUME" } };
        if (auto it = map.find(type); it != map.end()) {
            return it->second;
        }
        return "Invalid";
    }

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
        const std::vector<std::string> get_ordered_item_id_filtered_by_classify_type(const std::vector<ItemClassifyType>& types) noexcept;

    protected:
        virtual bool parse(const json::value& json) override;
        void clear();

        // key：材料编号Id，value：材料名（zh，utf8）
        std::unordered_map<std::string, std::string> m_item_name;
        std::unordered_set<std::string> m_all_item_id;
        // 对于std::unordered_map<std::string,int>，
        // key：材料编号Id，value：sortId
        std::unordered_map<ItemClassifyType, std::unordered_map<std::string,int>>
            m_all_item_id_with_sortId_filtered_by_classify_type;
    };

    inline static auto& ItemData = ItemConfig::get_instance();
}
