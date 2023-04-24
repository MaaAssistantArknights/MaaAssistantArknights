#pragma once

#include "Common/AsstTypes.h"
#include "Utils/NoWarningCVMat.h"

#include <variant>

MAA_NS_BEGIN

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
    };

public:
    OCRerConfig() = default;
    virtual ~OCRerConfig() = default;

    void set_params(Params params);

    void set_required(std::vector<std::string> required) noexcept;
    void set_replace(const std::unordered_map<std::string, std::string>& replace, bool replace_full = false) noexcept;

    virtual void set_task_info(std::shared_ptr<TaskInfo> task_ptr);
    virtual void set_task_info(const std::string& task_name);

    virtual void set_use_char_model(bool enable) noexcept;

protected:
    virtual void _set_roi(const Rect& roi) = 0;
    virtual void _set_task_info(OcrTaskInfo task_info);

protected:
    Params m_params;
};

MAA_NS_END
