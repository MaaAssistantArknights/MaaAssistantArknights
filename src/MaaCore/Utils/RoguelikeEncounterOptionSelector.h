#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace asst::RoguelikeEncounterOptionSelector
{
struct PreferredOption
{
    std::vector<std::string> texts;
    size_t occurrence = 1; // 1-based occurrence among visible options.
};

struct Rule
{
    std::string id;
    std::vector<std::string> option_texts;
    std::vector<PreferredOption> preferred_options;
    std::vector<std::string> safe_fallback_options;
};

struct VisibleOption
{
    bool enabled = false;
    std::string text;
};

struct Result
{
    std::optional<size_t> index;
    bool matched_variant = false;
    bool used_safe_fallback = false;
    std::string rule_id;
};

inline bool contains_option_multiset(const std::vector<VisibleOption>& options, const std::vector<std::string>& texts)
{
    if (options.size() > texts.size()) {
        return false;
    }

    std::vector<bool> used(texts.size(), false);
    for (const auto& option : options) {
        bool found = false;
        for (size_t index = 0; index < texts.size(); ++index) {
            if (used[index] || texts[index] != option.text) {
                continue;
            }
            used[index] = true;
            found = true;
            break;
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

inline std::optional<size_t> find_preferred_option(
    const std::vector<VisibleOption>& options,
    const std::vector<std::string>& texts,
    size_t occurrence)
{
    if (occurrence == 0) {
        return std::nullopt;
    }

    for (const auto& text : texts) {
        size_t seen = 0;
        for (size_t index = 0; index < options.size(); ++index) {
            if (!options[index].enabled || options[index].text != text) {
                continue;
            }
            ++seen;
            if (seen == occurrence) {
                return index;
            }
        }
    }
    return std::nullopt;
}

inline std::optional<size_t> find_safe_option(
    const std::vector<VisibleOption>& options,
    const std::vector<std::string>& texts)
{
    for (const auto& text : texts) {
        for (size_t index = 0; index < options.size(); ++index) {
            if (options[index].enabled && options[index].text == text) {
                return index;
            }
        }
    }
    return std::nullopt;
}

inline Result select(const std::vector<VisibleOption>& options, const Rule& base, const std::vector<Rule>& variants)
{
    const Rule* selected = &base;
    for (const auto& variant : variants) {
        if (!contains_option_multiset(options, variant.option_texts)) {
            continue;
        }
        selected = &variant;
        break;
    }

    const auto use_rule = [&](const Rule& rule, bool is_variant, bool fallback) -> Result {
        for (const auto& preferred : rule.preferred_options) {
            if (auto index = find_preferred_option(options, preferred.texts, preferred.occurrence);
                index && options[*index].enabled) {
                return Result { index, is_variant, fallback, rule.id };
            }
        }
        if (auto index = find_safe_option(options, rule.safe_fallback_options)) {
            return Result { index, is_variant, true, rule.id };
        }
        return Result { std::nullopt, is_variant, fallback, rule.id };
    };

    const bool selected_variant = selected != &base;
    Result result = use_rule(*selected, selected_variant, false);
    if (!result.index && selected != &base) {
        result = use_rule(base, false, false);
    }
    return result;
}
} // namespace asst::RoguelikeEncounterOptionSelector
