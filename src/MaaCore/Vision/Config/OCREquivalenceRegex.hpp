#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace asst
{
namespace equivalence_regex_detail
{
inline constexpr bool is_regex_metachar(char ch) noexcept
{
    switch (ch) {
    case '\\':
    case '.':
    case '^':
    case '$':
    case '|':
    case '?':
    case '*':
    case '+':
    case '(':
    case ')':
    case '[':
    case ']':
    case '{':
    case '}':
        return true;
    default:
        return false;
    }
}

// A branch of `(?:a|b|c)`, where every regex metacharacter has to be escaped to stay literal.
inline std::string escape_alternation_member(std::string_view member)
{
    std::string escaped;
    escaped.reserve(member.size());
    for (const char ch : member) {
        if (is_regex_metachar(ch)) {
            escaped += '\\';
        }
        escaped += ch;
    }
    return escaped;
}

// A member of `[...]`. `\`, `]`, `^` and `-` are escaped unconditionally, so that a member always keeps its
// literal meaning no matter where it ends up inside the class.
inline std::string escape_class_member(std::string_view member)
{
    std::string escaped;
    escaped.reserve(member.size());
    for (const char ch : member) {
        if (ch == '\\' || ch == ']' || ch == '^' || ch == '-') {
            escaped += '\\';
        }
        escaped += ch;
    }
    return escaped;
}

// Appends a class member as a literal, skipping members that are already part of the class.
inline void append_class_member(std::string_view member, std::vector<std::string>& appended, std::string& expanded)
{
    if (std::ranges::find(appended, member) != appended.end()) {
        return;
    }
    appended.emplace_back(member);
    expanded += escape_class_member(member);
}

struct EquivalenceMatch
{
    const std::vector<std::string>* members = nullptr;
    const std::string* member = nullptr; // the member that matched, kept first when expanding
    std::size_t length = 0;
};

// Looks up the longest equivalence class member matching `pattern` at `pos`. Ties keep the order of the
// classes. Members are single characters in every shipped configuration, but nothing here assumes that.
inline EquivalenceMatch
    find_match(std::string_view pattern, std::size_t pos, const std::vector<std::vector<std::string>>& classes)
{
    EquivalenceMatch match;
    for (const auto& eq_class : classes) {
        if (eq_class.size() <= 1) {
            continue;
        }
        for (const auto& member : eq_class) {
            if (member.empty() || member.size() <= match.length) {
                continue;
            }
            if (pattern.substr(pos, member.size()) == member) {
                match = { &eq_class, &member, member.size() };
            }
        }
    }
    return match;
}
} // namespace equivalence_regex_detail

// Expands OCR equivalence classes in an `ocrReplace` key. Such a key is a regex pattern, while equivalence
// classes are plain character sets, so the expansion has to respect the structure of the regex. A plain text
// replacement used to inject regex syntax into character classes: `[Oo]` became `[O(?:o|о)]`, which made `:`
// a member of the class and turned the OCR result `03:53:4` into `0305304`
// (https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/15084).
//
// - Outside a character class a match is replaced by an alternation of all its equivalents:
//   `o` -> `(?:o|O|о)`, escaped as literals.
// - Inside a character class the equivalents are inserted as escaped literals: `[Oo]` -> `[Ooо]`.
// - Ranges are kept as they are: `-` between two class members is a range operator, so `[A-Z]` stays `[A-Z]`
//   instead of becoming a corrupted range or an over-broad class. The characters at both ends of a range are
//   kept verbatim for the same reason.
// - Escape sequences such as `\d` or `\]` are atomic and never expanded.
//
// The pattern is scanned once, so inserted members are never expanded again. If nothing matches, the result
// is byte-for-byte identical to the input.
inline std::string
    expand_equivalence_in_regex(std::string_view pattern, const std::vector<std::vector<std::string>>& classes)
{
    using namespace equivalence_regex_detail;

    std::string expanded;
    expanded.reserve(pattern.size());

    bool in_class = false;
    bool at_class_start = false;            // only `[` or `[^` has been written, so `]` is still a literal member
    bool range_pending = false;             // a range operator has been written, its end character is still to come
    std::vector<std::string> class_members; // members already written to the current class, for de-duplication

    for (std::size_t pos = 0; pos < pattern.size();) {
        const char ch = pattern[pos];

        if (ch == '\\' && pos + 1 < pattern.size()) {
            // An escape sequence is atomic: `\]` does not close a class, and `\d` is not the member `d`.
            expanded.append(pattern, pos, 2);
            pos += 2;
            at_class_start = false;
            range_pending = false;
            continue;
        }

        if (!in_class) {
            if (ch == '[') {
                in_class = true;
                at_class_start = true;
                range_pending = false;
                class_members.clear();
                expanded += ch;
                ++pos;
                if (pos < pattern.size() && pattern[pos] == '^') { // negation, not a member
                    expanded += '^';
                    ++pos;
                }
                continue;
            }

            const auto match = find_match(pattern, pos, classes);
            if (match.members != nullptr) {
                expanded += "(?:";
                for (std::size_t index = 0; index < match.members->size(); ++index) {
                    if (index != 0) {
                        expanded += '|';
                    }
                    expanded += escape_alternation_member(match.members->at(index));
                }
                expanded += ')';
                pos += match.length;
                continue;
            }

            expanded += ch;
            ++pos;
            continue;
        }

        // Inside a character class.
        if (ch == ']' && !at_class_start) {
            in_class = false;
            expanded += ch;
            ++pos;
            continue;
        }

        if (ch == '-' && !at_class_start && pos + 1 < pattern.size() && pattern[pos + 1] != ']') {
            // A range operator, as in `[A-Z]`. It is neither a member nor the end of the class.
            expanded += ch;
            ++pos;
            range_pending = true;
            continue;
        }

        if (range_pending) {
            // The end of a range is kept verbatim, expanding it would change what the range matches.
            expanded += ch;
            ++pos;
            range_pending = false;
            at_class_start = false;
            continue;
        }

        const auto match = find_match(pattern, pos, classes);
        if (match.members != nullptr) {
            const auto member_end = pos + match.length;
            const bool is_range_start =
                member_end + 1 < pattern.size() && pattern[member_end] == '-' && pattern[member_end + 1] != ']';
            if (is_range_start) {
                // The start of a range is kept verbatim, expanding it would change what the range matches.
                expanded.append(pattern, pos, match.length);
                pos = member_end;
                at_class_start = false;
                continue;
            }

            // The matched member is written first, the remaining equivalents follow the order of the class.
            append_class_member(*match.member, class_members, expanded);
            for (const auto& member : *match.members) {
                if (&member != match.member) {
                    append_class_member(member, class_members, expanded);
                }
            }
            pos = member_end;
            at_class_start = false;
            continue;
        }

        expanded += ch;
        ++pos;
        at_class_start = false;
    }

    return expanded;
}
} // namespace asst
