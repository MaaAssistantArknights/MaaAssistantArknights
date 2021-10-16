#pragma once
#include "AbstractConfiger.h"

#include <unordered_map>
#include <unordered_set>

namespace asst {
    struct InfrastSkill {
        std::string id;
        std::string templ_name;
        std::vector<std::string> names;  // 很多基建技能是一样的，就是名字不同。所以一个技能id可能对应多个名字
        std::string intro;
    };

    class InfrastConfiger : public AbstractConfiger
    {
    public:
        virtual ~InfrastConfiger() = default;

        const std::unordered_set<std::string>& get_templ_required() const noexcept {
            return m_templ_required;
        }
        const std::unordered_map<std::string, InfrastSkill>& get_skills() const noexcept {
            return m_skills;
        }

    protected:
        virtual bool parse(const json::value& json) override;

        std::unordered_map<std::string, InfrastSkill> m_skills;
        std::unordered_set<std::string> m_templ_required;
    };
}
