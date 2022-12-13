#pragma once

#include <string>

namespace Map
{
    struct LevelKey
    {
        std::string stageId;
        std::string code;
        std::string levelId;
        std::string name;

        bool empty_or_equal(const std::string& lhs, const std::string& rhs) const noexcept
        {
            return (lhs.empty() || rhs.empty()) ? true : lhs == rhs;
        }
        bool operator==(const LevelKey& other) const noexcept
        {
            return empty_or_equal(stageId, other.stageId) && empty_or_equal(code, other.code) &&
                   empty_or_equal(levelId, other.levelId) && empty_or_equal(name, other.name);
        }
        bool operator==(const std::string& any_key) const noexcept
        {
            if (any_key.empty()) {
                return false;
            }
            return empty_or_equal(stageId, any_key) || empty_or_equal(code, any_key) ||
                   empty_or_equal(levelId, any_key) || empty_or_equal(name, any_key);
        }
    };
}
