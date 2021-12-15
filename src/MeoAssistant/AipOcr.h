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

        AipOcr& get_instance()
        {
            static AipOcr unique_instance;
            return unique_instance;
        }

        bool request_access_token(const std::string& client_id, const std::string& client_secret);
        std::vector<TextRect> request_ocr_accurate(const cv::Mat& image, const TextRectProc& pred = nullptr);

    private:
        AipOcr() = default;
        AipOcr(const AipOcr&) = default;
        AipOcr(AipOcr&&) noexcept = default;

        std::vector<TextRect> request_ocr_and_parse(std::string_view cmd_fmt, const cv::Mat& image, const TextRectProc& pred = nullptr);
        std::vector<TextRect> parse_response(const json::value& json, const TextRectProc& pred = nullptr);

        std::string m_access_token;
    };
}
