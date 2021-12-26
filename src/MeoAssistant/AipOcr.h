#pragma once

#include <iostream>

#include "AsstDef.h"

namespace cv
{
    class Mat;
}
namespace json
{
    class value;
}

namespace asst
{
    class AipOcr
    {
    public:
        ~AipOcr() = default;

        static AipOcr& get_instance()
        {
            static AipOcr unique_instance;
            return unique_instance;
        }

        bool request_access_token(const std::string& client_id, const std::string& client_secret);
        bool request_ocr_general(const cv::Mat image, std::vector<TextRect>& out_result, const TextRectProc& pred = nullptr);
        bool request_ocr_accurate(const cv::Mat image, std::vector<TextRect>& out_result, const TextRectProc& pred = nullptr);

    private:
        AipOcr() = default;
        AipOcr(const AipOcr&) = default;
        AipOcr(AipOcr&&) noexcept = default;

        bool request_ocr_and_parse(std::string_view cmd_fmt, const cv::Mat image, std::vector<TextRect>& out_result, const TextRectProc& pred = nullptr);
        bool parse_response(const json::value& json, std::vector<TextRect>& out_result, const TextRectProc& pred = nullptr);

        std::string m_access_token;
    };
}
