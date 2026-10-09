#pragma once
#include "Task/AbstractTask.h"
#include <map>

namespace asst
{
// Files are supplied in preference order by the client. Progress is read from the game.
class ParadoxAutoTask : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    bool set_files(const std::vector<std::pair<int, std::string>>& files);

private:
    struct Candidate
    {
        int id;
        std::string filename;
    };

    enum class CandidateResult
    {
        Completed,
        AlreadyCompleted,
        Failed,
    };

    bool _run() override;
    CandidateResult run_candidate(const Candidate& candidate, bool from_detail);
    void report(const std::string& what, const std::string& name, int id = -1);

    std::map<std::string, std::vector<Candidate>> m_candidates;
};
}
