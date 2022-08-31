#pragma once

#include "AbstractTask.h"
#include "AsstUtils.hpp"

#include <future>
#include <regex>
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
            std::unordered_map<std::string_view, std::string_view> m_headers;
            bool _analyze_status_line(std::string_view status_line)
            {
                size_t __word_count = 0;
                for (const std::string_view& word :
                     std::views::split(status_line, ' ') | std::views::transform([](auto str_range) {
                         return utils::view_cast<std::string_view>(str_range);
                     })) {
                    ++__word_count;
                    if (__word_count == 1) {
                        static const std::unordered_set<std::string> accepted_protocol_version = { "HTTP/1.1" };
                        if (!accepted_protocol_version.contains(utils::view_cast<std::string>(word))) {
                            m_last_error = "unsupport protocol version";
                            return false;
                        }
                        m_protocol_version = utils::view_cast<std::string>(word);
                    }
                    else if (__word_count == 2) {
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
                if (__word_count == 2) {
                    m_status_code_info = "";
                    return true;
                }
                m_last_error = "status line too short";
                return false;
            }
            bool _analyze_headers_line(std::string_view status_line)
            {
                size_t colon_pos = status_line.find(':');
                if (colon_pos == status_line.npos) {
                    m_last_error = "connot decode header `" + std::string(status_line) + "`";
                    return false;
                }
                m_headers[status_line.substr(0, colon_pos)] =
                    status_line.substr(colon_pos + 1 + status_line.substr(colon_pos + 1).find_first_not_of(' '));
                return true;
            }

        public:
            template <typename... Args>
            requires std::is_constructible_v<std::string, Args...>
            Response(Args&&... args) : std::string(std::forward<Args>(args)...)
            {
                bool __is_status_line = true;
                for (const auto& line : std::views::split(*this, '\n')) {
                    std::string_view line_view;
                    if (utils::view_cast<std::string_view>(line).ends_with('\r')) {
                        line_view = utils::view_cast<std::string_view>(line | std::views::take(line.size() - 1));
                    }
                    else {
                        line_view = utils::view_cast<std::string_view>(line);
                    }
                    if (__is_status_line) {
                        if (!_analyze_status_line(line_view)) {
                            return;
                        }
                        __is_status_line = false;
                    }
                    else if (!line_view.empty()) {
                        if (!_analyze_headers_line(line_view)) {
                            return;
                        }
                    }
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
            const std::unordered_map<std::string_view, std::string_view> headers() const { return m_headers; }

            bool success() const { return m_status_code == 200; }
            bool code_2xx() const { return m_status_code >= 200 && m_status_code < 300; }
            bool code_3xx() const { return m_status_code >= 300 && m_status_code < 400; }
            bool code_4xx() const { return m_status_code >= 400 && m_status_code < 500; }
            bool code_5xx() const { return m_status_code >= 500 && m_status_code < 600; }
        };

    protected:
        virtual bool _run() override;

        void report_to_penguin();
        void report_to_yituliu();
        Response escape_and_request(const std::string& subtask, const std::string& format);

        ReportType m_report_type = ReportType::Invaild;
        std::string m_body;
        std::string m_extra_param;
        std::vector<std::future<void>> m_upload_pending;
    };
} // namespace asst
