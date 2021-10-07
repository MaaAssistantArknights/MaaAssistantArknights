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

        //template<typename RequiredArray>
        //void set_required(RequiredArray required, bool full_match) noexcept {
        //    static_assert(std::is_constructible<std::string, required::value_type>::value,
        //        "Parameter can't be used to construct a std::string");
        //    m_required.assign(std::make_move_iterator(required.begin()), std::make_move_iterator(required.end()));
        //    m_full_match = full_match;
        //}
        void set_required(std::vector<std::string> required, bool full_match = false) noexcept {
            m_required = std::move(required);
            m_full_match = full_match;
        }

        //template<typename ReplaceMap>
        //void set_replace(ReplaceMap replace) noexcept {
        //    static_assert(std::is_constructible<m_replace::value_type, required::value_type>::value,
        //        "Parameter can't be used to construct a replace map");
        //    std::unordered_map<std::string, std::string> temp_map(
        //        std::make_move_iterator(replace.begin()), std::make_move_iterator(replace.end());
        //    m_replace.swap(temp_map);
        //}
        void set_replace(std::unordered_map<std::string, std::string> replace) noexcept {
            m_replace = std::move(replace);
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
