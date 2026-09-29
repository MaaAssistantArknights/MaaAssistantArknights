#pragma once
#include "Task/AbstractTask.h"

#include "Common/AsstBattleDef.h"
#include "Vision/Oper/OperBoxImageAnalyzer.h"

namespace asst
{
class BattleProcessTask;

class ParadoxRecognitionTask : public AbstractTask
{
private:
    struct OperName
    {
        battle::Role role = battle::Role::Unknown;
        int rarity = 0;
        std::string name;
        std::string name_en;
        std::string name_jp;
        std::string name_kr;
        std::string name_tw;
    };

public:
    using AbstractTask::AbstractTask;
    virtual ~ParadoxRecognitionTask() override = default;
    void add_file(int id, const std::string& navigate_name);

    void set_from_detail(bool enabled) { m_from_detail = enabled; }

    void set_battle_task_ptr(const std::shared_ptr<BattleProcessTask>& ptr) { m_battle_task_ptr = ptr; }

private:
    virtual bool _run() override;
    void return_to_oper_list() const;
    bool match_oper(const std::string& oper_name) const; // oper_name 和 m_navigate_name 匹配
    static std::string standardize_name(const std::string& navigate_name);
    bool enter_paradox(int skill_num, int rarity);
    void report_status(const std::string& status);

    std::vector<std::pair<int, std::string>> m_paradox_files;
    OperName m_oper_name {};
    bool m_from_detail = false;
    std::string m_navigate_name;
    int m_skill_num;
    std::shared_ptr<BattleProcessTask> m_battle_task_ptr = nullptr;
};
}
