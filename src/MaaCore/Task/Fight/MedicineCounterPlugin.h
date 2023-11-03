#pragma once
#include "Task/AbstractTaskPlugin.h"

#include "Common/AsstTypes.h"

namespace asst
{
    class MedicineCounterPlugin final : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~MedicineCounterPlugin() override = default;
        virtual bool verify(AsstMsg msg, const json::value& details) const override;
        void set_dr_grandet(bool dr_grandet) { m_dr_grandet = dr_grandet; };
        void set_count(int count) { m_max_count = count; }
        void set_use_expiring(bool use_expiring) { m_use_expiring = use_expiring; }

    private:
        virtual bool _run() override;

        enum class ExpiringStatus
        {
            UnSure = 0,
            NotExpiring = 1,
            Expiring = 2,
        };
        struct Medicine
        {
            int use = 0;
            int inventry = 0;
            asst::Rect reduce_button_position;
            ExpiringStatus is_expiring;
        };

        // 库存量, 移除按钮的位置
        struct InitialMedicineResult
        {
            int using_count = 0;
            std::vector<Medicine> medicines;
        };

        // 识别使用的药量
        std::optional<InitialMedicineResult> init_count(cv::Mat image);
        // 减少药品使用
        void reduce_excess(const InitialMedicineResult& using_medicine);
        std::optional<int> get_target_of_sanity(const cv::Mat& image);
        std::optional<int> get_maximun_of_sanity(const cv::Mat& image);

        bool m_use_expiring = false;
        bool m_dr_grandet = false;
        int m_using_count = 0;
        int m_max_count = 0;
    };
}
