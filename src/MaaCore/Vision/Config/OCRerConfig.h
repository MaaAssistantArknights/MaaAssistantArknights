#pragma once

#include "Common/AsstTypes.h"
#include "Utils/NoWarningCVMat.h"

#include <variant>

MAA_VISION_NS_BEGIN

class OCRerConfig
{
public:
    struct Params
    {
        std::vector<std::string> required;
        bool full_match = false;
        std::unordered_map<std::string, std::string> replace;
        bool replace_full = false;
        bool without_det = false;
        bool use_char_model = false;

        int bin_threshold_lower = 140;
        int bin_threshold_upper = 255;
        int bin_expansion = 2;
    };

public:
    OCRerConfig() = default;
    virtual ~OCRerConfig() = default;

    void set_params(Params params);

    void set_required(std::vector<std::string> required) noexcept;
    void set_replace(const std::unordered_map<std::string, std::string>& replace, bool replace_full = false) noexcept;

    virtual void set_task_info(std::shared_ptr<TaskInfo> task_ptr);
    virtual void set_task_info(const std::string& task_name);

    void set_use_char_model(bool enable) noexcept;

    void set_bin_threshold(int lower, int upper = 255);
    void set_bin_expansion(int expansion);

protected:
    virtual void _set_roi(const Rect& roi) = 0;
    virtual void _set_task_info(OcrTaskInfo task_info);

protected:
    Params m_params;
};

MAA_VISION_NS_END
