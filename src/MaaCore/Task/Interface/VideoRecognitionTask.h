#pragma once
#include "Task/InterfaceTask.h"
#include "Task/Miscellaneous/StartGameTaskPlugin.h"

namespace asst
{
class ProcessTask;

class VideoRecognitionTask final : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "VideoRecognition";

    VideoRecognitionTask(const AsstCallback& callback, Assistant* inst);
    virtual ~VideoRecognitionTask() override = default;

    bool set_params(const json::value& params) override;
};
}
