#pragma once

#include "Common/AsstTypes.h"
#include "Utils/NoWarningCVMat.h"

#include <variant>

namespace asst
{
class OCRerConfig
{
public:
    struct Params
    {
        std::vector<std::pair<std::string, std::string>> required; // raw, equivalent
        bool full_match = false;
        std::vector<std::pair<std::string, std::string>> replace;
        bool replace_full = false;
        bool without_det = false;
        bool use_char_model = false;

        int bin_threshold_lower = 140;
        int bin_threshold_upper = 255;
        int bin_expansion = 2;
        int bin_left_trim_threshold = 0;
        int bin_right_trim_threshold = 0;
    };

public:
    OCRerConfig() = default;
    virtual ~OCRerConfig() = default;

    void set_params(Params params);

    void set_required(std::vector<std::string> required) noexcept;
    void set_replace(
        const std::vector<std::pair<std::string, std::string>>& replace,
        bool replace_full = false) noexcept;

    virtual void set_task_info(std::shared_ptr<TaskInfo> task_ptr);
    virtual void set_task_info(const std::string& task_name);

    void set_without_det(bool without_det) noexcept;
    void set_use_char_model(bool enable) noexcept;

    void set_bin_threshold(int lower, int upper = 255);
    void set_bin_expansion(int expansion);
    void set_bin_trim_threshold(int left, int right);

protected:
    virtual void _set_roi(const Rect& roi) = 0;
    virtual void _set_task_info(OcrTaskInfo task_info);

protected:
    Params m_params;
};
}
