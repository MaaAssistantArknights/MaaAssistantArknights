#pragma once
#include "AbstractImageAnalyzer.h"

#include <vector>
#include <unordered_map>
#include <functional>

#include "AsstDef.h"

namespace asst {
    class OcrImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~OcrImageAnalyzer() = default;

        virtual bool analyze() override;

        void set_required(std::vector<std::string> required, bool full_match = false) noexcept {
            m_required = std::move(required);
            m_full_match = full_match;
        }
        void set_replace(std::unordered_map<std::string, std::string> replace) noexcept {
            m_replace = std::move(replace);
        }
        void set_task_info(OcrTaskInfo task_info) noexcept {
            m_required = std::move(task_info.text);
            m_full_match = task_info.need_full_match;
            m_replace = std::move(task_info.replace_map);
            set_roi(task_info.roi);
        }

        void set_pred(const TextRectProc& pred) {
            m_pred = pred;
        }
        const std::vector<TextRect>& get_result() const noexcept {
            return m_ocr_result;
        }

    protected:
        std::vector<TextRect> m_ocr_result;
        std::vector<std::string> m_required;
        bool m_full_match = false;
        std::unordered_map<std::string, std::string> m_replace;
        TextRectProc m_pred = nullptr;
    };
}
