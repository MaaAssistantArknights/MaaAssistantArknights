#pragma once

#include "InfrastAbstractTask.h"

namespace asst {
    // 识别干员任务，会将识别到的信息写入文件中
    class IdentifyOperTask : public InfrastAbstractTask
    {
    public:
        IdentifyOperTask(AsstCallback callback, void* callback_arg);
        virtual ~IdentifyOperTask() = default;

        virtual bool run() override;

    protected:
        // 一边滑动一边识别
        std::optional<std::unordered_map<std::string, OperInfrastInfo>> swipe_and_detect();

        // 检测干员精英化等级
        int detect_elite(const cv::Mat& image, const asst::Rect name_rect);
    };
}
