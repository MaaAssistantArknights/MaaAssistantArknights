#pragma once

#include "AbstractConfiger.h"

#include <unordered_map>

namespace asst {
    class ItemConfiger : public AbstractConfiger
    {
    public:
        virtual ~ItemConfiger() = default;

        static ItemConfiger& get_instance() {
            static ItemConfiger unique_instance;
            return unique_instance;
        }

        // key：材料编号Id，value：材料名（zh，utf8）
        std::unordered_map<std::string, std::string> m_item_name;
        // key：材料编号Id，value：数量
        std::unordered_map<std::string, int> m_drop_count;

        void clear_drop_count();

    protected:
        ItemConfiger() = default;
        ItemConfiger(const ItemConfiger& rhs) = default;
        ItemConfiger(ItemConfiger&& rhs) noexcept = default;

        ItemConfiger& operator=(const ItemConfiger& rhs) = default;
        ItemConfiger& operator=(ItemConfiger&& rhs) noexcept = default;

        virtual bool parse(const json::value& json) override;
    };
}
