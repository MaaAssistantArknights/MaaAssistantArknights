#pragma once

#include "AsstRanges.hpp"
#include "AsstUtils.hpp"

#include <cctype>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace asst::http
{
    class Response : public std::string
    {
    private:
        std::string m_last_error;
        unsigned m_status_code = 0;
        std::string_view m_protocol_version;
        std::string_view m_status_code_info;
        std::string_view m_data;
        std::unordered_map<std::string, std::string_view> m_headers;
        bool _analyze_status_line(std::string_view status_line)
        {
            size_t _word_count = 0;
            for (const auto& word : utils::string_split(status_line, " ")) {
                ++_word_count;
                if (_word_count == 1) {
                    static const std::unordered_set<std::string_view> accepted_protocol_version = { "HTTP/1.1" };
                    if (!accepted_protocol_version.contains(word)) {
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
                    m_status_code_info = std::string_view(word.begin(), status_line.end());
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
                m_last_error = "connot decode header `" + std::string(status_line) + "`";
                return false;
            }
            m_headers[utils::view_cast<std::string>(status_line.substr(0, _colon_pos) | views::transform([](char c) {
                                                        return static_cast<char>(std::tolower(c));
                                                    }))] =
                status_line.substr(_colon_pos + 1 + status_line.substr(_colon_pos + 1).find_first_not_of(' '));
            return true;
        }

    public:
        template <typename... Args>
        requires std::is_constructible_v<std::string, Args...>
        Response(Args&&... args) : std::string(std::forward<Args>(args)...)
        {
            std::string_view this_view = *this;
            bool _is_status_line = true;
            for (const auto& line : utils::string_split(this_view, "\r\n")) {
                // status
                if (_is_status_line) {
                    if (!_analyze_status_line(line)) {
                        return;
                    }
                    _is_status_line = false;
                }
                // headers
                else if (!line.empty()) {
                    if (!_analyze_headers_line(line)) {
                        return;
                    }
                }
                // data
                else {
                    m_data = std::string_view(line.begin(), this_view.end());
                    return;
                }
            }
        }

        unsigned status_code() const { return m_status_code; }
        std::string_view protocol_version() const { return m_protocol_version; }
        std::string_view status_code_info() const { return m_status_code_info; }
        std::string_view data() const { return m_data; }
        std::optional<std::string_view> find_header(const std::string& key) const
        {
            if (auto iter = m_headers.find(key); iter != m_headers.cend()) {
                return iter->second;
            }
            else {
                return std::nullopt;
            }
        }
        const auto& headers() const { return m_headers; }
        const std::string& get_last_error() const noexcept { return m_last_error; }

        bool success() const { return m_status_code == 200; }
        bool status_2xx() const { return m_status_code >= 200 && m_status_code < 300; }
        bool status_3xx() const { return m_status_code >= 300 && m_status_code < 400; }
        bool status_4xx() const { return m_status_code >= 400 && m_status_code < 500; }
        bool status_5xx() const { return m_status_code >= 500 && m_status_code < 600; }
    };
} // namespace asst::http
