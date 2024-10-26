#pragma once
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
class TaskFileReloadTask : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~TaskFileReloadTask() override = default;
    void set_magic_code(std::string code);
    void set_path(std::string path);

private:
    virtual bool _run() override;
    std::string m_file;
    bool is_path = false, is_magic_code = false;
};
}
