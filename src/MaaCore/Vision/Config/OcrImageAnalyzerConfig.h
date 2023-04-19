#pragma once

#include "Common/AsstTypes.h"
#include "Utils/NoWarningCVMat.h"

#include <variant>

namespace asst
{
    class OcrImageAnalyzerConfig
    {
    public:
        OcrImageAnalyzerConfig() = default;
        virtual ~OcrImageAnalyzerConfig() = default;

        void set_required(std::vector<std::string> required) noexcept;
        void set_replace(const std::unordered_map<std::string, std::string>& replace,
                         bool replace_full = false) noexcept;

        virtual void set_task_info(std::shared_ptr<TaskInfo> task_ptr);
        virtual void set_task_info(const std::string& task_name);

        virtual void set_use_char_model(bool enable) noexcept;

    protected:
        virtual void _set_roi(const Rect& roi) = 0;
        virtual void _set_task_info(OcrTaskInfo task_info);

        std::vector<std::string> m_required;
        bool m_full_match = false;
        std::unordered_map<std::string, std::string> m_replace;
        bool m_replace_full = false;
        bool m_without_det = false;
        bool m_use_char_model = false;
    };
}
