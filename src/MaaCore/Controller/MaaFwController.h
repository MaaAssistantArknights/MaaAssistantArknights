#pragma once

#include <functional>
#include <optional>
#include <string>
#include <utility>

#include "Common/AsstMsg.h"
#include "ControllerAPI.h"
#include "InstHelper.h"
#include "MaaFwControlUnitInterface.h"
#include "Platform/PlatformFactory.h"
#include "Utils/LibraryHolder.hpp"

namespace asst
{
class Assistant;

class MaaFwController : public ControllerAPI, private InstHelper, public LibraryHolder
{
public:
    MaaFwController(const AsstCallback& callback, Assistant* inst, PlatformType type);
    ~MaaFwController() override;

    MaaFwController(const MaaFwController&) = delete;
    MaaFwController& operator=(const MaaFwController&) = delete;
    MaaFwController(MaaFwController&&) = delete;
    MaaFwController& operator=(MaaFwController&&) = delete;

    bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;
    bool inited() const noexcept override;

    const std::string& get_uuid() const override;

    size_t get_pipe_data_size() const noexcept override { return 114'514; }

    size_t get_version() const noexcept override { return 114'514; }

    bool screencap(cv::Mat& image_payload, bool allow_reconnect = false) override;
    bool screencap_for_preview(cv::Mat& image_payload, bool allow_reconnect = false) override;

    bool start_game(const std::string& client_type) override;
    bool stop_game(const std::string& client_type) override;

    bool click(const Point& p) override;
    bool input(const std::string& text) override;
    bool swipe(
        const Point& p1,
        const Point& p2,
        int duration = 0,
        SwipeExtraDirection extra_swipe = SwipeExtraDirection::None,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false) override;

    bool inject_input_event(const InputEvent& event) override;
    bool press_esc() override;

    void set_main_screen_recognition(bool on) override { m_main_screen_recognition = on; }

    ControlFeat::Feat support_features() const noexcept override;
    std::pair<int, int> get_screen_res() const noexcept override;

protected:
    bool connect_with_extras(
        const std::string& adb_path,
        const std::string& address,
        const std::string& config,
        const json::object& extras);

private:
    enum class ControlUnitType
    {
        MaaAdbControlUnit,
        MaaAndroidNativeControlUnit,
        MaaLinuxControlUnit,
        MEOJSON_ENUM_RANGE(MaaAdbControlUnit, MaaLinuxControlUnit)
    };

    bool init_library(ControlUnitType control_unit_type);
    void destroy_unit();
    void callback(AsstMsg msg, const json::value& details) const;
    bool screencap_impl(cv::Mat& image_payload, bool move_cursor);

    MaaFwControlUnitAPI* m_unit_handle = nullptr;
    // MaaFramework/include/MaaControlUnit/ControlUnitAPI.h
    using GetVersionFunc = const char*();
    using DestroyFunc = void(MaaFwControlUnitAPI*);
    std::function<GetVersionFunc> m_get_version_func;
    std::function<DestroyFunc> m_destroy_func;

    bool m_inited = false;
    std::optional<ControlUnitType> m_control_unit_type;
    std::string m_library_name;
    int m_esc_keycode = 0;
    bool m_target_is_pc = false;
    std::string m_uuid;
    std::pair<int, int> m_screen_size = { 0, 0 };
    bool m_main_screen_recognition = false;

    AsstCallback m_callback = nullptr;
};
} // namespace asst
