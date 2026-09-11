#include "OcrConfig.h"

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"
#include "Vision/Config/OCREquivalenceRegex.hpp"

std::string asst::OcrConfig::process_equivalence_class(const std::string& str) const
{
    std::string result = str;
    for (const auto& eq_class : m_eq_classes) {
        std::ranges::for_each(eq_class.begin() + 1, eq_class.end(), [&](const std::string& elem) {
            utils::string_replace_all_in_place(result, elem, eq_class.front());
        });
    }
    return result;
}

bool asst::OcrConfig::parse(const json::value& json)
{
    LogTraceFunction;

    std::vector<equivalence_class> eq_classes;
    for (const json::value& eq_class : json.at("equivalence_classes").as_array()) {
        equivalence_class eq_class_tmp;
        for (const json::value& eq_element : eq_class.as_array()) {
            std::string member = eq_element.as_string();
            if (!equivalence_regex_detail::is_single_unicode_scalar(member)) {
                Log.error("equivalence class member must be a single Unicode scalar", member);
                return false;
            }
            eq_class_tmp.emplace_back(std::move(member));
        }
        eq_classes.emplace_back(std::move(eq_class_tmp));
    }
    m_eq_classes = std::move(eq_classes);
    return true;
}
