#pragma once

#include "AbstractConfiger.h"

#include <string>
#include <vector>
#include <unordered_set>

#include "AsstDef.h"

namespace asst {
    // 干员信息，公开招募相关
    struct OperRecruitInfo {
        std::string name;
        std::string type;
        int level = 0;
        std::string sex;
        std::unordered_set<std::string> tags;
        bool hidden = false;
        std::string name_en;
    };

    // 公开招募的干员组合
    struct OperRecruitCombs {
        std::vector<OperRecruitInfo> opers;
        int max_level = 0;
        int min_level = 0;
        double avg_level = 0;
    };

    class RecruitConfiger : public AbstractConfiger
    {
    public:
        virtual ~RecruitConfiger() = default;
        constexpr static int CorrectNumberOfTags = 5;

        const std::unordered_set<std::string>& get_all_tags() const noexcept
        {
            return m_all_tags;
        }
        const std::vector<OperRecruitInfo>& get_all_opers() const noexcept
        {
            return m_all_opers;
        }

    protected:
        virtual bool parse(const json::value& json) override;

        std::unordered_set<std::string> m_all_tags;
        std::unordered_set<std::string> m_all_types;

        std::vector<OperRecruitInfo> m_all_opers;
    };
}