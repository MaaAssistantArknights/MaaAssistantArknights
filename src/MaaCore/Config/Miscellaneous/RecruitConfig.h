#pragma once

#include "Config/AbstractConfig.h"

#include "Utils/Ranges.hpp"
#include <algorithm>
#include <numeric>
#include <string>
#include <unordered_set>
#include <vector>

#include "Common/AsstTypes.h"

namespace asst
{
    // 干员信息，公开招募相关
    struct Recruitment
    {
        std::string name;
        int level = 0;
        std::unordered_set<std::string> tags;

        bool has_tag(const std::string& tag) const { return tags.contains(tag); }

        friend std::strong_ordering operator<=>(const Recruitment& lhs, const Recruitment& rhs)
        {
            if (lhs.level != rhs.level) return lhs.level <=> rhs.level; // increment order
#ifdef __clang__
            if (rhs.name < lhs.name)
                return std::strong_ordering::less;
            else if (rhs.name > lhs.name)
                return std::strong_ordering::greater;
            else
                return std::strong_ordering::equal;
#else
            return rhs.name <=> lhs.name; // decrement order (print reversely)
#endif
        }

        friend bool operator==(const Recruitment& lhs, const Recruitment& rhs)
        {
            return lhs.name == rhs.name && lhs.level == rhs.level;
        }
    };

    // 公开招募的干员组合
    struct RecruitCombs
    {
        // TODO: using vector here can be expensive
        std::vector<std::string> tags;
        std::vector<Recruitment> opers;
        int max_level = 0;
        int min_level = 0;
        double avg_level = 0;

        void update_attributes()
        {
            min_level = std::transform_reduce(
                opers.cbegin(), opers.cend(), 7, [](int a, int b) -> int { return (std::min)(a, b); },
                std::mem_fn(&Recruitment::level));

            max_level = std::transform_reduce(
                opers.cbegin(), opers.cend(), 0, [](int a, int b) -> int { return (std::max)(a, b); },
                std::mem_fn(&Recruitment::level));

            avg_level = std::transform_reduce(opers.cbegin(), opers.cend(), 0., std::plus<double> {},
                                              std::mem_fn(&Recruitment::level)) /
                        static_cast<double>(opers.size());
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

    class RecruitConfig final : public SingletonHolder<RecruitConfig>, public AbstractConfig
    {
    public:
        using TagId = std::string;

    public:
        virtual ~RecruitConfig() override = default;
        static constexpr int CorrectNumberOfTags = 5;

        const std::unordered_set<std::string>& get_all_tags() const noexcept { return m_all_tags; }
        const std::vector<Recruitment>& get_all_opers() const noexcept { return m_all_opers; }
        std::string get_tag_name(const TagId& id) const noexcept;

    protected:
        virtual bool parse(const json::value& json) override;

        void clear();

        std::unordered_set<std::string> m_all_tags;
        std::vector<Recruitment> m_all_opers;
        std::unordered_map<TagId, std::string> m_all_tags_name;
    };
    inline static auto& RecruitData = RecruitConfig::get_instance();
} // namespace asst
