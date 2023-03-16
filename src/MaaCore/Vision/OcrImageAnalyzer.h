#pragma once
#include "AbstractImageAnalyzer.h"

#include <functional>
#include <unordered_map>
#include <vector>

#include "Common/AsstTypes.h"
#include "Config/Miscellaneous/OcrPack.h"

namespace asst
{
    class OcrImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        using Result = OcrPack::Result;
        using ResultVector = OcrPack::ResultVector;
        using ResultProc = OcrPack::ResultProc;

        enum class Sorting
        {
            None,
            ByScore,      // 按分数排序，得分最高的在前面
            ByRequired,   // 按传入的需求数组排序，传入的在前面结果接在前面
            ByHorizontal, // 按位置排序，右上角在左下角前面
            ByVertical,   // 按位置排序，左下角在右上角前面
        };

    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~OcrImageAnalyzer() override = default;

        const std::optional<ResultVector>& analyze() const;
        const std::optional<ResultVector>& result() const { return m_result; }

        void set_pred(const ResultProc& pred);

        void set_required(std::vector<std::string> required) noexcept;
        void set_replace(const std::unordered_map<std::string, std::string>& replace,
                         bool replace_full = false) noexcept;
        void set_sorting(Sorting s);

        virtual void set_task_info(std::shared_ptr<TaskInfo> task_ptr);
        virtual void set_task_info(const std::string& task_name);

        virtual void set_use_cache(bool is_use) noexcept;
        virtual void set_region_of_appeared(Rect region) noexcept;
        virtual void set_use_char_model(bool enable) noexcept;

    protected:
        virtual void set_task_info(OcrTaskInfo task_info) noexcept;

        void sort_(ResultVector& res, Sorting method) const;
        void sort_by_score_(ResultVector& res) const;
        void sort_by_required_(ResultVector& res) const;
        void sort_by_horizontal_(ResultVector& res) const;
        void sort_by_vertical_(ResultVector& res) const;

        std::vector<std::string> m_required;
        bool m_full_match = false;
        std::unordered_map<std::string, std::string> m_replace;
        bool m_replace_full = false;
        ResultProc m_pred = nullptr;
        Sorting m_sorting = Sorting::ByScore;
        bool m_without_det = false;
        bool m_use_cache = false;
        Rect m_region_of_appeared;
        bool m_use_char_model = false;

        mutable std::optional<ResultVector> m_result;
    };
}
