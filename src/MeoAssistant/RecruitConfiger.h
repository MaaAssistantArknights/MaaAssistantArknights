#pragma once

#include "AbstractConfiger.h"

#include <string>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <numeric>
#include "AsstRanges.hpp"

#include "AsstTypes.h"

namespace asst
{
    // 干员信息，公开招募相关
    struct RecruitOperInfo
    {
        std::string name;
        std::string type;
        int level = 0;
        std::string sex;
        std::unordered_set<std::string> tags;
        bool hidden = false;
        std::string name_en;

        bool has_tag(const std::string& tag) const
        {
            return tags.contains(tag);
        }

        friend std::strong_ordering operator<=>(const RecruitOperInfo& lhs, const RecruitOperInfo& rhs)
        {
            if (lhs.level != rhs.level) return lhs.level <=> rhs.level; // increment order
            return rhs.name <=> lhs.name; // decrement order (print reversely)
        }

        friend bool operator==(const RecruitOperInfo& lhs, const RecruitOperInfo& rhs)
        {
            return lhs.name == rhs.name && lhs.level == rhs.level;
        }
    };

    // 公开招募的干员组合
    struct RecruitCombs
    {
        // TODO: using vector here can be expensive
        std::vector<std::string> tags;
        std::vector<RecruitOperInfo> opers;
        int max_level = 0;
        int min_level = 0;
        double avg_level = 0;

        void update_attributes()
        {
            min_level = std::transform_reduce(
                    opers.cbegin(), opers.cend(), 7,
                    [](int a, int b) -> int { return (std::min)(a, b); },
                    std::mem_fn(&RecruitOperInfo::level));

            max_level = std::transform_reduce(
                    opers.cbegin(), opers.cend(), 0,
                    [](int a, int b) -> int { return (std::max)(a, b); },
                    std::mem_fn(&RecruitOperInfo::level));

            avg_level = std::transform_reduce(
                    opers.cbegin(), opers.cend(), 0.,
                    std::plus<double>{},
                    std::mem_fn(&RecruitOperInfo::level)) / static_cast<double>(opers.size());
        }

        // intersection of two recruit combs
        friend RecruitCombs operator*(RecruitCombs& lhs, RecruitCombs& rhs)
        {
            ranges::sort(lhs.tags);
            ranges::sort(lhs.opers);
            ranges::sort(rhs.tags);
            ranges::sort(rhs.opers);

            RecruitCombs result;

            ranges::set_union(lhs.tags, rhs.tags, std::back_inserter(result.tags));

            ranges::set_intersection(lhs.opers, rhs.opers, std::back_inserter(result.opers));

            result.update_attributes();

            return result;
        }
    };

    class RecruitConfiger : public AbstractConfiger
    {
    public:
        virtual ~RecruitConfiger() override = default;
        constexpr static int CorrectNumberOfTags = 5;

        const std::unordered_set<std::string>& get_all_tags() const noexcept
        {
            return m_all_tags;
        }
        const std::vector<RecruitOperInfo>& get_all_opers() const noexcept
        {
            return m_all_opers;
        }

    protected:
        virtual bool parse(const json::value& json) override;

        void clear();

        std::unordered_set<std::string> m_all_tags;
        std::unordered_set<std::string> m_all_types;

        std::vector<RecruitOperInfo> m_all_opers;
    };
}
