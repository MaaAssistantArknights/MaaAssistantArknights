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

// True iff `text` is exactly one Unicode scalar value encoded as UTF-8. Equivalence class members are
// required to be single scalars: the shipped `ocr_config.json` files only contain that, and
// `OcrConfig::parse()` rejects anything else.
inline bool is_single_unicode_scalar(std::string_view text) noexcept
{
    if (text.empty()) {
        return false;
    }

    const auto byte = [text](std::size_t index) {
        return static_cast<unsigned char>(text[index]);
    };

    const unsigned char lead = byte(0);
    std::size_t length = 0;
    char32_t code_point = 0;
    if (lead <= 0x7F) {
        length = 1;
        code_point = lead;
    }
    else if (lead >= 0xC2 && lead <= 0xDF) {
        length = 2;
        code_point = lead & 0x1F;
    }
    else if (lead >= 0xE0 && lead <= 0xEF) {
        length = 3;
        code_point = lead & 0x0F;
    }
    else if (lead >= 0xF0 && lead <= 0xF4) {
        length = 4;
        code_point = lead & 0x07;
    }
    else {
        return false;
    }

    if (text.size() != length) {
        return false;
    }
    for (std::size_t index = 1; index < length; ++index) {
        const unsigned char cont = byte(index);
        if (cont < 0x80 || cont > 0xBF) {
            return false;
        }
        code_point = (code_point << 6) | (cont & 0x3F);
    }
    if (length == 3) {
        if (lead == 0xE0 && byte(1) < 0xA0) {
            return false; // overlong
        }
        if (lead == 0xED && byte(1) >= 0xA0) {
            return false; // UTF-16 surrogates
        }
    }
    if (length == 4) {
        if (lead == 0xF0 && byte(1) < 0x90) {
            return false; // overlong
        }
        if (lead == 0xF4 && byte(1) > 0x8F) {
            return false; // above U+10FFFF
        }
    }
    return code_point <= 0x10FFFF && (code_point < 0xD800 || code_point > 0xDFFF);
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
    if (!is_single_unicode_scalar(member) || std::ranges::find(appended, member) != appended.end()) {
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

// Looks up the longest single-scalar member matching `pattern` at `pos`. Ties keep the order of the classes.
// Multi-scalar members are ignored: they are rejected when the configuration is loaded, and the expander
// must not pretend to support them (a longest match of `"ab"` expanded as `(?:a|ab)` is not the string `"ab"`).
inline EquivalenceMatch
    find_match(std::string_view pattern, std::size_t pos, const std::vector<std::vector<std::string>>& classes)
{
    EquivalenceMatch match;
    for (const auto& eq_class : classes) {
        if (eq_class.size() <= 1) {
            continue;
        }
        for (const auto& member : eq_class) {
            if (!is_single_unicode_scalar(member) || member.size() <= match.length) {
                continue;
            }
            if (pattern.substr(pos, member.size()) == member) {
                match = { &eq_class, &member, member.size() };
            }
        }
    }
    return match;
}

// `\Q...\E` quotes a literal span. Missing `\E` quotes through the end of the pattern.
inline bool consume_quoted_span(std::string_view pattern, std::size_t& pos, std::string& expanded)
{
    if (pos + 1 >= pattern.size() || pattern[pos] != '\\' || pattern[pos + 1] != 'Q') {
        return false;
    }

    std::size_t end = pos + 2;
    while (end < pattern.size()) {
        if (pattern[end] == '\\' && end + 1 < pattern.size() && pattern[end + 1] == 'E') {
            end += 2;
            expanded.append(pattern, pos, end - pos);
            pos = end;
            return true;
        }
        ++end;
    }

    expanded.append(pattern, pos, pattern.size() - pos);
    pos = pattern.size();
    return true;
}

// POSIX `[:name:]`, collating `[.ch.]` and equivalence `[=ch=]` inside a character class.
inline bool consume_posix_construct(std::string_view pattern, std::size_t& pos, std::string& expanded)
{
    if (pos + 1 >= pattern.size() || pattern[pos] != '[') {
        return false;
    }
    const char delim = pattern[pos + 1];
    if (delim != ':' && delim != '.' && delim != '=') {
        return false;
    }
    for (std::size_t index = pos + 2; index + 1 < pattern.size(); ++index) {
        if (pattern[index] == delim && pattern[index + 1] == ']') {
            expanded.append(pattern, pos, index + 2 - pos);
            pos = index + 2;
            return true;
        }
    }
    return false;
}

inline constexpr bool is_extension_flag_char(char ch) noexcept
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '-' || ch == '^';
}

// Copies the Boost/Perl `(?...)` introducer. Group contents after the introducer are left for the main
// scan, so `(?=-)` can still expand `-` while `(?s:.)` does not rewrite the `s` flag.
inline void consume_extension_prefix(std::string_view pattern, std::size_t& pos, std::string& expanded)
{
    expanded += "(?";
    pos += 2;
    if (pos >= pattern.size()) {
        return;
    }

    const auto copy_until = [&](char end) {
        while (pos < pattern.size() && pattern[pos] != end) {
            if (pattern[pos] == '\\' && pos + 1 < pattern.size()) {
                expanded.append(pattern, pos, 2);
                pos += 2;
                continue;
            }
            expanded += pattern[pos];
            ++pos;
        }
        if (pos < pattern.size()) {
            expanded += pattern[pos];
            ++pos;
        }
    };

    const char kind = pattern[pos];
    switch (kind) {
    case ':': // (?:
    case '=': // (?=
    case '!': // (?!
    case '>': // (?>
        expanded += kind;
        ++pos;
        return;
    case '<': // (?<=  (?<!  (?<name>
        expanded += '<';
        ++pos;
        if (pos < pattern.size() && (pattern[pos] == '=' || pattern[pos] == '!')) {
            expanded += pattern[pos];
            ++pos;
            return;
        }
        copy_until('>');
        return;
    case '\'': // (?'name'
        expanded += '\'';
        ++pos;
        copy_until('\'');
        return;
    case '#': // (?#comment)
        expanded += '#';
        ++pos;
        copy_until(')');
        return;
    case 'P': // (?P<name>  (?P=name)
        expanded += 'P';
        ++pos;
        if (pos < pattern.size() && (pattern[pos] == '<' || pattern[pos] == '=')) {
            const char end = pattern[pos] == '<' ? '>' : ')';
            expanded += pattern[pos];
            ++pos;
            copy_until(end);
        }
        return;
    case '(': // (?(cond)
        expanded += '(';
        ++pos;
        copy_until(')');
        return;
    default:
        while (pos < pattern.size() && is_extension_flag_char(pattern[pos])) {
            expanded += pattern[pos];
            ++pos;
        }
        if (pos < pattern.size() && (pattern[pos] == ':' || pattern[pos] == ')')) {
            expanded += pattern[pos];
            ++pos;
        }
        return;
    }
}

inline void emit_alternation(const EquivalenceMatch& match, std::string& expanded)
{
    expanded += "(?:";
    bool first = true;
    const auto emit = [&](const std::string& member) {
        if (!is_single_unicode_scalar(member)) {
            return;
        }
        if (!first) {
            expanded += '|';
        }
        first = false;
        expanded += escape_alternation_member(member);
    };

    emit(*match.member);
    for (const auto& member : *match.members) {
        if (&member != match.member) {
            emit(member);
        }
    }
    expanded += ')';
}
} // namespace equivalence_regex_detail

// Expands OCR equivalence classes in an `ocrReplace` key. Such a key is a regex pattern, while equivalence
// classes are plain character sets, so the expansion has to respect the structure of the regex. A plain text
// replacement used to inject regex syntax into character classes: `[Oo]` became `[O(?:o|о)]`, which made `:`
// a member of the class and turned the OCR result `03:53:4` into `0305304`
// (https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/15084).
//
// - Outside a character class a match is replaced by an alternation of all its equivalents:
//   `o` -> `(?:o|O|о)`, escaped as literals. The matched member is kept first.
// - Inside a character class the equivalents are inserted as escaped literals: `[Oo]` -> `[Ooо]`.
// - Ranges are kept as they are: `-` between two class members is a range operator, so `[A-Z]` stays `[A-Z]`.
// - Control structure is copied atomically and never expanded: escape sequences, `\Q...\E`, POSIX
//   `[:name:]` / `[.ch.]` / `[=ch=]`, and the introducer of a `(?...)` extension.
// - Members are a single Unicode scalar value. Multi-scalar members are ignored here and rejected by
//   `OcrConfig::parse()`.
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
        if (consume_quoted_span(pattern, pos, expanded)) {
            at_class_start = false;
            range_pending = false;
            continue;
        }

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

            if (ch == '(' && pos + 1 < pattern.size() && pattern[pos + 1] == '?') {
                consume_extension_prefix(pattern, pos, expanded);
                continue;
            }

            const auto match = find_match(pattern, pos, classes);
            if (match.members != nullptr) {
                emit_alternation(match, expanded);
                pos += match.length;
                continue;
            }

            expanded += ch;
            ++pos;
            continue;
        }

        // Inside a character class.
        if (consume_posix_construct(pattern, pos, expanded)) {
            at_class_start = false;
            range_pending = false;
            continue;
        }

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
