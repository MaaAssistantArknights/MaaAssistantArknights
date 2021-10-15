#pragma once
#include "AbstractImageAnalyzer.h"

#include "AsstDef.h"

namespace asst {
    enum class SmileyType {
        Invalid = -1,
        Rest,       // 休息完成，绿色笑脸
        Work,       // 工作中，黄色笑脸
        Distract    // 注意力涣散，红色哭脸
    };
    struct SmileyInfo {
        SmileyType type;
        Rect rect;
    };

    class InfrastSmileyImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~InfrastSmileyImageAnalyzer() = default;

        virtual bool analyze() override;

        const std::vector<SmileyInfo>& get_result() const noexcept {
            return m_result;
        }
    protected:
        std::vector<SmileyInfo> m_result;
    };
}
