#pragma once

#include <cstdint>

#include "Task/AbstractTask.h"

namespace asst
{
class GreenTicketShoppingTask final : public AbstractTask
{
public:
    GreenTicketShoppingTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);

    void set_material(std::string item_id, int count)
    {
        m_item_id = std::move(item_id);
        m_count = count;
    }

    static std::optional<int> read_remaining_stock(const cv::Mat& image);

    bool is_shortfall_filled() const noexcept { return m_purchased_count >= m_count; }

protected:
    bool _run() override;

private:
    bool run_task(const std::string& name, int retry_times = RetryTimesDefault);

    std::string m_item_id;
    int m_count = 0;
    std::int64_t m_purchased_count = 0;
};
}
