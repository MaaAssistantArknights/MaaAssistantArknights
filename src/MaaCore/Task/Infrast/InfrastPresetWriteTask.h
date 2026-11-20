#pragma once

#include "InfrastAbstractTask.h"
#include <array>
#include <optional>

namespace asst
{
// Only edits saved presets. No station/swap action is used by this task.
class InfrastPresetWriteTask final : public InfrastAbstractTask
{
public:
    using InfrastAbstractTask::InfrastAbstractTask;
    bool configure(const json::value& params);

    std::string facility_name() const override { return m_facility; }

    size_t max_num_of_opers() const noexcept override { return m_capacity; }

protected:
    bool _run() override;
    bool on_run_fails() override;

private:
    struct Room
    {
        std::string facility;
        int index = 0;
        std::array<std::vector<std::string>, 3> queues;
    };

    struct Page
    {
        std::string title;
        std::string facility;
        int number = 0;
        int total = 0;
        int count = 0;
    };

    std::optional<Page> read_page() const;
    bool move_room(const Page& page, int direction);
    bool same_room(const Page& expected) const;
    bool scroll_presets(const Page& page, bool down);
    std::optional<Rect> find_text(const Rect& roi, const std::string& text) const;
    bool open_queue(const Page& page, int queue);
    bool ensure_queue(const Page& page, int queue);
    bool verify_selection(const std::vector<std::string>& names);
    bool write_queue(const Page& page, int queue, const std::vector<std::string>& names);
    void report(const std::string& stage, const std::string& room = {}, int queue = 0);

    std::vector<Room> m_rooms;
    std::string m_facility;
    size_t m_capacity = 5;
    std::string m_current_room;
    int m_current_queue = 0;
    int m_verified = 0;
};
}
