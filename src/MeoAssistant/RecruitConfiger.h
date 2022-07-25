#pragma once

#include "AbstractConfiger.h"

#include <string>
#include <unordered_set>
#include <vector>
#include <algorithm>

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

        [[nodiscard]] bool has_tag(const std::string &tag) const {
            return tags.find(tag) != tags.cend();
        }

        friend bool operator<(const RecruitOperInfo& lhs, const RecruitOperInfo& rhs)
        {
            if (lhs.level != rhs.level) return lhs.level > rhs.level;
            return lhs.name < rhs.name;
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

        void update_attributes() {
            double sum = 0;
            min_level = 7;
            max_level = 0;
            for (const auto& op : opers) {
                sum += op.level;
                min_level = (std::min)(min_level, op.level);
                max_level = (std::max)(max_level, op.level);
            }
            avg_level = sum / static_cast<double>(opers.size());
        }

        // intersection of two recruit combs
        friend RecruitCombs operator*(RecruitCombs& lhs, RecruitCombs& rhs)
        {
            std::sort(lhs.tags.begin(), lhs.tags.end());
            std::sort(lhs.opers.begin(), lhs.opers.end());
            std::sort(rhs.tags.begin(), rhs.tags.end());
            std::sort(rhs.opers.begin(), rhs.opers.end());

            RecruitCombs result;

            std::set_union(
                    lhs.tags.cbegin(), lhs.tags.cend(),
                    rhs.tags.cbegin(), rhs.tags.cend(),
                    std::back_inserter(result.tags));

            std::set_intersection(
                    lhs.opers.cbegin(), lhs.opers.cend(),
                    rhs.opers.cbegin(), rhs.opers.cend(),
                    std::back_inserter(result.opers));

            result.update_attributes();

            return result;
        }
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
