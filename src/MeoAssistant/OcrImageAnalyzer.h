#pragma once
#include "AbstractImageAnalyzer.h"

#include <functional>
#include <unordered_map>
#include <vector>

#include "AsstTypes.h"

namespace asst
{
    class OcrImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~OcrImageAnalyzer() = default;

        virtual bool analyze() override;

        void filter(const TextRectProc& filter_func);

        void sort_result();             // 按位置排序，左上角的排在前面，右上角在左下角前面
        void sort_result_x_y();         // 按位置排序，左上角的排在前面，左下角在右上角前面
        void sort_result_by_score();    // 按分数排序，得分最高的在前面
        void sort_result_by_required(); // 按传入的需求数组排序，传入的在前面结果接在前面

        void set_use_cache(bool is_use) noexcept;
        void set_required(std::vector<std::string> required, bool full_match = false) noexcept;
        void set_replace(std::unordered_map<std::string, std::string> replace) noexcept;
        void set_task_info(std::shared_ptr<TaskInfo> task_ptr);
        void set_task_info(const std::string& task_name);
        void set_region_of_appeared(Rect region) noexcept;

        void set_pred(const TextRectProc& pred);
        const std::vector<TextRect>& get_result() const noexcept;

    protected:
        void set_task_info(OcrTaskInfo task_info) noexcept;

        std::vector<TextRect> m_ocr_result;
        std::vector<std::string> m_required;
        bool m_full_match = false;
        std::unordered_map<std::string, std::string> m_replace;
        TextRectProc m_pred = nullptr;
        bool m_without_det = false;
        bool m_use_cache = false;
        Rect m_region_of_appeared;
    };
}
