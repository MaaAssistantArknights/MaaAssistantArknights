#pragma once

#include <string>
#include <string_view>
#include <unordered_set>

namespace asst
{
inline bool is_main_screen_task(std::string_view name, const std::unordered_set<std::string>& themed_entry_tasks)
{
    while (true) {
        if (name == "Award" || name == "Mall" || name == "Visit" || name == "Infrast" || name == "Recruit" ||
            name == "Fight" || name == "Depot" || name == "OperBox" || name == "Gacha" ||
            themed_entry_tasks.contains(std::string(name))) {
            return true;
        }

        const auto at = name.find('@');
        if (at == std::string_view::npos) {
            return false;
        }
        name.remove_prefix(at + 1);
    }
}
}
