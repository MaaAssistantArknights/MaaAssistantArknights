#pragma once
#include "Common/AsstBattleDef.h"
#include "Task/AbstractTask.h"
#include "Vision/Miscellaneous/OperImageAnalyzer.h"
#include <unordered_set>

namespace asst
{
    /*struct OperBoxInfo
    {
        std::string name;
    };*/
    class OperRecognitionTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~OperRecognitionTask() override = default;

    protected:
        virtual bool _run() override;
        void swipe_page();
        //std::vector<OperBoxInfo> analyzer_opers();
        std::vector<OperBoxInfo> own_opers;
        void callback_analyze_result(bool done);
        std::unordered_set<std::string> get_own_oper_names();
        //std::filesystem::path oper_img_path = utils::path("debug") / utils::path("oper");
        //cv::Mat m_oper_image;
        bool swipe_and_analyze();
    };
}
