#pragma once

#include <cstddef>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <boost/regex.hpp>

namespace asst
{
// True iff `text` is exactly one Unicode scalar value encoded as UTF-8. Equivalence class members are
// required to be single scalars: the shipped `ocr_config.json` files only contain that, and
// `OcrConfig::parse()` rejects anything else. Multi-scalar members cannot be expressed by wchar_t traits
// (a longest match of `"ab"` is not the string `"ab"`), so they are refused rather than half-supported.
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

inline std::optional<wchar_t> utf8_scalar_to_wchar(std::string_view text) noexcept
{
    if (!is_single_unicode_scalar(text)) {
        return std::nullopt;
    }

    const auto byte = [text](std::size_t index) {
        return static_cast<unsigned char>(text[index]);
    };
    const unsigned char lead = byte(0);
    char32_t code_point = 0;
    std::size_t length = 1;
    if (lead <= 0x7F) {
        code_point = lead;
        length = 1;
    }
    else if (lead >= 0xC2 && lead <= 0xDF) {
        code_point = lead & 0x1F;
        length = 2;
    }
    else if (lead >= 0xE0 && lead <= 0xEF) {
        code_point = lead & 0x0F;
        length = 3;
    }
    else {
        code_point = lead & 0x07;
        length = 4;
    }
    for (std::size_t index = 1; index < length; ++index) {
        code_point = (code_point << 6) | (byte(index) & 0x3F);
    }

    if constexpr (sizeof(wchar_t) == 2) {
        if (code_point > 0xFFFF) {
            // Traits operate on a single wchar_t code unit. Supplementary-plane scalars need a UTF-16
            // surrogate pair and cannot be represented; shipped members are all BMP.
            return std::nullopt;
        }
    }
    return static_cast<wchar_t>(code_point);
}

// Boost.Regex traits that make OCR equivalence classes visible to the engine.
//
// Boost 1.74 `basic_regex` does not accept a traits *instance*, only a traits *type* default-constructed
// at compile time. The mapping therefore lives in process-wide static tables, filled when `OcrConfig`
// is loaded. Switching clients must call `set_classes` again (which also drops the regex cache).
//
// Matching pipeline (Boost 1.74, no `collate`):
// - Literals and single-character class members go through `translate` at compile time and match time.
// - Named POSIX classes (`[[:upper:]]`) resolve the class name via `lookup_classname` (not `translate`);
//   membership is `isctype(translate(input), mask)`.
// - Range endpoints (`[A-Z]`) are `translate`'d then stored as code-point intervals. That is the one
//   path traits cannot make equivalence-class aware, so resources rewrite `[A-Z]` / `[a-z]` to
//   `[[:upper:]]` / `[[:lower:]]`.
//
// `isctype(c, mask)`: if `c` belongs to an equivalence class, return true iff any member of that class
// satisfies the mask on the base traits. Otherwise fall back to the base implementation.
class OcrEquivalenceTraits : public boost::regex_traits<wchar_t>
{
public:
    using Base = boost::regex_traits<wchar_t>;
    using char_class_type = Base::char_class_type;

    wchar_t translate(wchar_t ch) const { return canonical(ch); }

    wchar_t translate_nocase(wchar_t ch) const { return this->tolower(canonical(ch)); }

    // The matcher calls the two-argument form. Both overloads have to be overridden, otherwise the
    // `icase` path skips equivalence.
    wchar_t translate(wchar_t ch, bool icase) const
    {
        ch = canonical(ch);
        return icase ? this->tolower(ch) : ch;
    }

    bool isctype(wchar_t ch, char_class_type mask) const
    {
        std::shared_lock lock(s_mutex);
        const auto members = members_of_locked(ch);
        if (members) {
            for (wchar_t member : *members) {
                if (Base::isctype(member, mask)) {
                    return true;
                }
            }
            return false;
        }
        lock.unlock();
        return Base::isctype(ch, mask);
    }

    static void set_classes(const std::vector<std::vector<std::string>>& classes);

private:
    static wchar_t canonical(wchar_t ch)
    {
        std::shared_lock lock(s_mutex);
        return canonical_locked(ch);
    }

    static wchar_t canonical_locked(wchar_t ch)
    {
        if (const auto it = s_canonical.find(ch); it != s_canonical.end()) {
            return it->second;
        }
        return ch;
    }

    static const std::vector<wchar_t>* members_of_locked(wchar_t ch)
    {
        const auto it = s_members.find(canonical_locked(ch));
        if (it == s_members.end() || it->second.size() <= 1) {
            return nullptr;
        }
        return &it->second;
    }

    inline static std::shared_mutex s_mutex;
    inline static std::unordered_map<wchar_t, wchar_t> s_canonical;
    inline static std::unordered_map<wchar_t, std::vector<wchar_t>> s_members;
};

using ocr_eq_wregex = boost::basic_regex<wchar_t, OcrEquivalenceTraits>;

namespace ocr_eq_cache_detail
{
inline std::shared_mutex& mutex()
{
    static std::shared_mutex mtx;
    return mtx;
}

inline std::unordered_map<std::wstring, ocr_eq_wregex>& cache()
{
    static std::unordered_map<std::wstring, ocr_eq_wregex> map;
    return map;
}
} // namespace ocr_eq_cache_detail

inline void clear_ocr_eq_regex_cache()
{
    std::unique_lock lock(ocr_eq_cache_detail::mutex());
    ocr_eq_cache_detail::cache().clear();
}

inline const ocr_eq_wregex& ocr_eq_regex(const std::wstring& pattern)
{
    {
        std::shared_lock slock(ocr_eq_cache_detail::mutex());
        auto& cache = ocr_eq_cache_detail::cache();
        if (auto it = cache.find(pattern); it != cache.end()) {
            return it->second;
        }
    }

    std::unique_lock ulock(ocr_eq_cache_detail::mutex());
    auto& cache = ocr_eq_cache_detail::cache();
    if (auto it = cache.find(pattern); it != cache.end()) {
        return it->second;
    }
    return cache.emplace(pattern, ocr_eq_wregex(pattern)).first->second;
}

inline void OcrEquivalenceTraits::set_classes(const std::vector<std::vector<std::string>>& classes)
{
    std::unordered_map<wchar_t, wchar_t> canonical;
    std::unordered_map<wchar_t, std::vector<wchar_t>> members;

    for (const auto& eq_class : classes) {
        if (eq_class.size() <= 1) {
            continue;
        }

        std::vector<wchar_t> class_members;
        class_members.reserve(eq_class.size());
        for (const auto& member : eq_class) {
            // Multi-scalar members are rejected by OcrConfig::parse() and ignored here. Traits map a
            // single wchar_t onto a single wchar_t; they cannot represent `"ab"` as one member.
            const auto converted = utf8_scalar_to_wchar(member);
            if (!converted) {
                continue;
            }
            class_members.push_back(*converted);
        }
        if (class_members.size() <= 1) {
            continue;
        }

        const wchar_t canon = class_members.front();
        for (wchar_t ch : class_members) {
            canonical.emplace(ch, canon);
        }
        members.emplace(canon, std::move(class_members));
    }

    {
        std::unique_lock lock(s_mutex);
        s_canonical = std::move(canonical);
        s_members = std::move(members);
    }
    clear_ocr_eq_regex_cache();
}
} // namespace asst
