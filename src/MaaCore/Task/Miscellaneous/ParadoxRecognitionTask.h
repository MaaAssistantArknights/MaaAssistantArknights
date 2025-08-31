#pragma once
#include "Common/AsstBattleDef.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Task/AbstractTask.h"
#include "Vision/Miscellaneous/OperBoxImageAnalyzer.h"

namespace asst
{
class ParadoxRecognitionTask : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~ParadoxRecognitionTask() override = default;
    void set_navigate_name(const std::string& navigate_name);

private:
    virtual bool _run() override;
    void swipe_page() const;                        // 翻页
    void return_initial_oper() const;               // 回到最左侧的干员
    bool click_role_table(battle::Role role) const; // 点击对应职业
    bool swipe_and_analyze();                       // 找干员
    std::optional<asst::Rect> match_from_result(const std::vector<OperBoxInfo>& result);
    bool match_oper(const std::string& oper_name);  // oper_name 和 m_navigate_name 匹配
    static std::string standardize_name(const std::string& navigate_name);
    void enter_paradox(int skill_num) const;        // 进悖论模拟

    json::object m_oper_name;
    std::string m_navigate_name;
    asst::Rect m_navigate_rect;
    int m_skill_num;
};
}
