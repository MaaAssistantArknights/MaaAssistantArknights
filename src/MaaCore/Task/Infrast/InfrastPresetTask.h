#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Common/AsstTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"
#include "Task/AbstractTask.h"

namespace asst
{
class InfrastPresetTask final : public AbstractTask
{
public:
    InfrastPresetTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
    virtual ~InfrastPresetTask() override = default;

    InfrastPresetTask& set_rooms(std::vector<std::string> rooms) noexcept;
    InfrastPresetTask& set_rest(bool rest) noexcept;

    struct RoomInfo
    {
        std::string id;
        std::string text;
        int order = 0;
    };

protected:
    virtual bool _run() override;
    virtual bool on_run_fails() override;

private:
    std::vector<RoomInfo> normalized_rooms() const;
    bool click_preset_buttons(std::vector<RoomInfo> rooms);
    std::unordered_map<std::string, Rect> analyze_visible_rooms(const cv::Mat& image, const std::vector<RoomInfo>& rooms) const;
    std::optional<Rect> find_enabled_switch_button(const cv::Mat& image, const Rect& room_text_rect) const;
    void exit_preset_page() const;
    void swipe_down() const;

    std::vector<std::string> m_rooms;
    bool m_rest = true;
};
} // namespace asst
