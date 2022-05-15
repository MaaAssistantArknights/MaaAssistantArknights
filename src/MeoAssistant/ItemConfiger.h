#pragma once

#include "AbstractConfiger.h"

#include <unordered_map>
#include <unordered_set>

namespace asst
{
    class ItemConfiger : public AbstractConfiger
    {
    public:
        virtual ~ItemConfiger() = default;

        const std::string& get_item_name(const std::string& id) const noexcept
        {
            if (auto iter = m_item_name.find(id);
                iter != m_item_name.cend()) {
                return iter->second;
            }
            else {
                static const std::string empty;
                return empty;
            }
        }
        const auto& get_all_item_id() const noexcept
        {
            return m_all_item_id;
        }

    protected:
        virtual bool parse(const json::value& json) override;

        // key：材料编号Id，value：材料名（zh，utf8）
        std::unordered_map<std::string, std::string> m_item_name;
        std::unordered_set<std::string> m_all_item_id;
    };
}
