#pragma once
#pragma once
#pragma once

#include "Config/AbstractConfigWithTempl.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace asst
{
    class RoleConfig final : public SingletonHolder<RoleConfig>, public AbstractConfigWithTempl
    {
    public:
        virtual ~RoleConfig() override = default;

        const auto& get_all_role_name() const noexcept { return role_all_name; }
        virtual const std::unordered_set<std::string>& get_templ_required() const noexcept override
        {
            return get_all_role_name();
        }
        const auto& get_role_tag() const noexcept { return tagname; }

    protected:
        virtual bool parse(const json::value& json) override;
        void clear();

        // 干员名（zh，utf8）
        std::unordered_set<std::string> role_all_name;
        std::string tagname;
    };

    inline static auto& RoleData = RoleConfig::get_instance();
}
