#pragma once

#include "Ranges.hpp"
#include "StringMisc.hpp"

#include <cctype>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace asst::http
{
    class Response
    {
    private:
        static const inline std::unordered_set<std::string_view> _accepted_protocol_version = { "HTTP/1.1",
                                                                                                "HTTP/2.0" };
        std::string m_raw_data;
        std::string m_last_error;
        unsigned m_status_code = 0;
        std::string_view m_protocol_version;
        std::string_view m_status_code_info;
        std::string_view m_body;
        std::unordered_map<std::string_view, std::string_view> m_headers;

        bool _analyze_status_line(std::string_view status_line)
        {
            size_t _word_count = 0;
            for (std::string_view word :
                 views::split(status_line, ' ') | views::transform([&](const auto& rng) -> std::string_view {
                     return utils::make_string_view(rng);
                 })) {
                ++_word_count;
                if (_word_count == 1) {
                    if (!_accepted_protocol_version.contains(word)) {
                        m_last_error = "unsupported protocol version`" + std::string(word) + "`";
                        return false;
                    }
                    m_protocol_version = word;
                }
                else if (_word_count == 2) {
                    if (word.length() != 3) {
                        m_last_error = "status code length is not 3";
                        return false;
                    }
                    if (!ranges::all_of(word, [](char c) -> bool { return std::isdigit(c); })) {
                        return false;
                    }
                    m_status_code = std::atoi(word.data());
                }
                else {
                    m_status_code_info = utils::make_string_view(word.begin(), status_line.end());
                    return true;
                }
            }
            if (_word_count == 2) {
                m_status_code_info = "";
                return true;
            }
            m_last_error = "status line too short";
            return false;
        }
        bool _analyze_headers_line(std::string_view status_line)
        {
            size_t _colon_pos = status_line.find(':');
            if (_colon_pos == status_line.npos) {
                m_last_error = "cannot decode header `" + std::string(status_line) + "`";
                return false;
            }
            // 把 key 转换为小写
            for (auto& c : status_line.substr(0, _colon_pos)) {
                const_cast<char&>(c) = static_cast<char&&>(std::tolower(c));
            }
            m_headers[status_line.substr(0, _colon_pos)] =
                status_line.substr(_colon_pos + 1 + status_line.substr(_colon_pos + 1).find_first_not_of(' '));
            return true;
        }

    public:
        template <typename... Args>
        requires std::is_constructible_v<std::string, Args...>
        Response(Args&&... args) : m_raw_data(std::forward<Args>(args)...)
        {
            constexpr std::string_view delim = "\r\n";
            std::string_view this_view = m_raw_data;
            bool _is_status_line = true;
            bool _is_data_line = false;
            auto make_view = [&](const auto& rng) -> std::string_view { return utils::make_string_view(rng); };
            for (std::string_view line : views::split(this_view, delim) | views::transform(make_view)) {
                // status
                if (_is_status_line) {
                    if (!_analyze_status_line(line)) {
                        return;
                    }
                    _is_status_line = false;
                }
                // headers
                else if (!_is_data_line) {
                    if (!line.empty()) {
                        if (!_analyze_headers_line(line)) {
                            return;
                        }
                    }
                    else {
                        _is_data_line = true; // 当前行是空行，则下一行开始都是 data 段
                    }
                }
                // data
                else {
                    m_body = utils::make_string_view(line.begin(), this_view.end());
                    return;
                }
            }
        }

        Response() = default;
        ~Response() noexcept = default;

        Response(Response&&) noexcept = default;
        Response& operator=(Response&&) noexcept = default;

        // string_view 类型的成员在复制时，其指向的 string 可能被析构
        // 虽然应该有更高效的复制方式，不过在这里还是直接调用 Args&&... 的构造函数
        Response(const Response&) = delete;
        Response& operator=(const Response&) = delete;

        unsigned status_code() const noexcept { return m_status_code; }
        std::string_view protocol_version() const noexcept { return m_protocol_version; }
        std::string_view status_code_info() const noexcept { return m_status_code_info; }
        std::string_view body() const noexcept { return m_body; }
        std::optional<std::string_view> find_header(std::string_view key) const
        {
            if (auto iter = m_headers.find(key); iter != m_headers.cend()) {
                return iter->second;
            }
            else {
                return std::nullopt;
            }
        }
        const auto& headers() const noexcept { return m_headers; }
        const std::string& get_last_error() const noexcept { return m_last_error; }
        operator std::string() const noexcept { return m_raw_data; }

        bool success() const noexcept { return m_status_code == 200; }
        bool status_2xx() const noexcept { return m_status_code >= 200 && m_status_code < 300; }
        bool status_3xx() const noexcept { return m_status_code >= 300 && m_status_code < 400; }
        bool status_4xx() const noexcept { return m_status_code >= 400 && m_status_code < 500; }
        bool status_5xx() const noexcept { return m_status_code >= 500 && m_status_code < 600; }
    };
} // namespace asst::http
