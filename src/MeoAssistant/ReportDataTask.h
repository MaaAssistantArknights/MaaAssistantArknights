#pragma once

#include "AbstractTask.h"
#include "AsstUtils.hpp"

#include <cctype>
#include <future>
#include <unordered_set>

namespace asst
{
    enum class ReportType
    {
        Invaild,
        PenguinStats,
        YituliuBigData,
    };

    class ReportDataTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~ReportDataTask() noexcept override = default;

        ReportDataTask& set_report_type(ReportType type);

        ReportDataTask& set_body(std::string body);
        ReportDataTask& set_extra_param(std::string extra_param);

    protected:
        class Response : public std::string
        {
        private:
            std::string m_last_error;
            size_t m_status_code = 0;
            std::string_view m_protocol_version;
            std::string_view m_status_code_info;
            std::string_view m_data;
            std::unordered_map<std::string, std::string_view> m_headers;
            bool _analyze_status_line(std::string_view status_line)
            {
                size_t _word_count = 0;
                for (const std::string_view& word :
                     views::split(status_line, ' ') | views::transform([](auto str_range) {
                         return utils::view_cast<std::string_view>(str_range);
                     })) {
                    ++_word_count;
                    if (_word_count == 1) {
                        static const std::unordered_set<std::string_view> accepted_protocol_version = { "HTTP/1.1" };
                        if (!accepted_protocol_version.contains(utils::view_cast<std::string_view>(word))) {
                            m_last_error = "unsupport protocol version";
                            return false;
                        }
                        m_protocol_version = utils::view_cast<std::string_view>(word);
                    }
                    else if (_word_count == 2) {
                        static const std::unordered_map<char, int> numbers = {
                            { '0', 0 }, { '1', 1 }, { '2', 2 }, { '3', 3 }, { '4', 4 },
                            { '5', 5 }, { '6', 6 }, { '7', 7 }, { '8', 8 }, { '9', 9 },
                        };
                        if (word.length() != 3) {
                            m_last_error = "status code length is not 3";
                            return false;
                        }
                        m_status_code = 0;
                        for (char num : word) {
                            if (!numbers.contains(num)) {
                                m_last_error = "status code is not number";
                                return false;
                            }
                            m_status_code = m_status_code * 10 + numbers.at(num);
                        }
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
                m_headers[utils::view_cast<std::string>(
                    status_line.substr(0, _colon_pos) |
                    views::transform([](char c) { return static_cast<char>(std::tolower(c)); }))] =
                    status_line.substr(_colon_pos + 1 + status_line.substr(_colon_pos + 1).find_first_not_of(' '));
                return true;
            }

        public:
            template <typename... Args>
            requires std::is_constructible_v<std::string, Args...>
            Response(Args&&... args) : std::string(std::forward<Args>(args)...)
            {
                bool _is_status_line = true;
                // 这里的 \r\n 处理很奇怪，因为 views::split 好像不支持 string，只能 char
                for (const auto& line : views::split(*this, '\n')) {
                    std::string_view line_view;
                    if (utils::view_cast<std::string_view>(line).ends_with('\r')) {
                        line_view = utils::view_cast<std::string_view>(line | views::take(line.size() - 1));
                    }
                    else {
                        line_view = utils::view_cast<std::string_view>(line);
                    }
                    // status
                    if (_is_status_line) {
                        if (!_analyze_status_line(line_view)) {
                            return;
                        }
                        _is_status_line = false;
                    }
                    // headers
                    else if (!line_view.empty()) {
                        if (!_analyze_headers_line(line_view)) {
                            return;
                        }
                    }
                    // data
                    else {
                        m_data = std::string_view(line.begin(), this->end());
                        return;
                    }
                }
            }

            size_t status_code() const { return m_status_code; }
            std::string_view protocol_version() const { return m_protocol_version; }
            std::string_view status_code_info() const { return m_status_code_info; }
            std::string_view data() const { return m_data; }
            std::string_view at_header(const std::string& key) const { return m_headers.at(key); }
            bool contains_header(const std::string& key) const { return m_headers.contains(key); }
            const std::unordered_map<std::string, std::string_view> headers() const { return m_headers; }
            const std::string& get_last_error() const noexcept { return m_last_error; }

            bool success() const { return m_status_code == 200; }
            bool status_2xx() const { return m_status_code >= 200 && m_status_code < 300; }
            bool status_3xx() const { return m_status_code >= 300 && m_status_code < 400; }
            bool status_4xx() const { return m_status_code >= 400 && m_status_code < 500; }
            bool status_5xx() const { return m_status_code >= 500 && m_status_code < 600; }
        };

    protected:
        virtual bool _run() override;

        void report_to_penguin();
        void report_to_yituliu();
        Response escape_and_request(const std::string& format);
        Response report_and_retry(
            const std::string& subtask, const std::string& format, int report_retry_times = 1,
            std::function<bool(const Response&)> retry_condition = [](const Response& response) {
                return response.status_5xx();
            });

        ReportType m_report_type = ReportType::Invaild;
        std::string m_body;
        std::string m_extra_param;
        static inline std::vector<std::future<void>> m_upload_pending;
    };
} // namespace asst
