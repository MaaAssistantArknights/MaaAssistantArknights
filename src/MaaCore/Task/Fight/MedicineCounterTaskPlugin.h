#pragma once
#include "Task/AbstractTaskPlugin.h"

#include "Common/AsstTypes.h"

namespace asst
{
class MedicineCounterTaskPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~MedicineCounterTaskPlugin() override = default;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;

    void set_dr_grandet(bool dr_grandet) { m_dr_grandet = dr_grandet; };

    void set_count(int count) { m_max_count = count; }

    void set_medicine_expiring_in_days(int medicine_expiring_in_days)
    {
        m_medicine_expiring_in_days = medicine_expiring_in_days;
    }

    void set_reduce_when_exceed(bool reduce) { m_reduce_when_exceed = reduce; }

    int get_used_count() const { return m_used_count; }

    static std::optional<int> get_target_of_sanity(const cv::Mat& image);
    static std::optional<int> get_maximun_of_sanity(const cv::Mat& image);

private:
    virtual bool _run() override;

    enum class ExpiringStatus
    {
        UnSure = 0,
        NotExpiring = 1,
        Expiring = 2,
    };

    static std::string expiring_status_to_string(ExpiringStatus status)
    {
        switch (status) {
        case ExpiringStatus::UnSure:
            return "UnSure";
        case ExpiringStatus::NotExpiring:
            return "NotExpiring";
        case ExpiringStatus::Expiring:
            return "Expiring";
        default:
            return "Unknown";
        }
    }

    struct Medicine
    {
        int use = 0;
        int inventory = 0;
        asst::Rect reduce_button_position;
        ExpiringStatus is_expiring;
    };

    // 库存量, 移除按钮的位置
    struct MedicineResult
    {
        int using_count = 0;
        std::vector<Medicine> medicines;
    };

    // 识别使用的药量
    std::optional<MedicineResult> init_count(const cv::Mat& image) const;
    // 减少药品使用
    void reduce_excess(const MedicineResult& using_medicine, int reduce);

    int m_medicine_expiring_in_days = 0; // 吃几天之内过期的药, 0表示不使用过期药品
    bool m_dr_grandet = false;
    int m_used_count = 0;
    int m_max_count = 0;
    mutable bool m_has_used_medicine = false; // 是否开过药品页面
    bool m_reduce_when_exceed = false;        // 第一次开药品页面时, 超理智上限减少用药
};
}
