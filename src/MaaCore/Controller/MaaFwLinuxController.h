#pragma once

#ifdef __linux__ // Linux or Android

#include "Common/AsstMsg.h"
#include "ControllerAPI.h"
#include "InstHelper.h"
#include "MaaFwControlUnitInterface.h"
#include "Platform/PlatformIO.h"
#include "Utils/LibraryHolder.hpp"

namespace asst
{

struct MaaFwLinuxControlUnitLoader : LibraryHolder<MaaFwLinuxControlUnitLoader>
{
    bool loaded() const noexcept { return m_loaded; }

    bool load(const std::filesystem::path& dll_path)
    {
        if (m_loaded) {
            return false;
        }

        m_loaded = load_library(dll_path);

        if (!m_loaded) {
            Log.error("Failed to load library:", dll_path);
            return false;
        }

        m_get_version = get_function<const char*()>("MaaLinuxControlUnitGetVersion");
        m_create = get_function<MaaFwControlUnitAPI*(const char*)>("MaaLinuxControlUnitCreate");
        m_destroy = get_function<void(MaaFwControlUnitAPI*)>("MaaLinuxControlUnitDestroy");

        return m_get_version && m_create && m_destroy;
    }

    MaaFwControlUnitAPI* create(const char* config_json) const
    {
        if (!m_loaded || !m_create) {
            Log.error("Library not loaded or create function not available");
            return nullptr;
        }

        return m_create(config_json);
    }

    void destroy(MaaFwControlUnitAPI* handle) const
    {
        if (!m_loaded || !m_destroy) {
            Log.error("Library not loaded or destroy function not available");
            return;
        }

        if (handle) {
            m_destroy(handle);
        }
    }

    const char* get_version() const
    {
        if (!m_loaded || !m_get_version) {
            Log.error("Library not loaded or get_version function not available");
            return nullptr;
        }

        return m_get_version();
    }

private:
    bool m_loaded = false;

    std::function<MaaFwControlUnitAPI*(const char*)> m_create;
    std::function<void(MaaFwControlUnitAPI*)> m_destroy;
    std::function<const char*()> m_get_version;
};

class MaaFwLinuxController : public ControllerAPI, private InstHelper
{
public:
    MaaFwLinuxController(const MaaFwLinuxController&) = delete;
    MaaFwLinuxController(MaaFwLinuxController&&) = delete;
    MaaFwLinuxController& operator=(const MaaFwLinuxController&) = delete;
    MaaFwLinuxController& operator=(MaaFwLinuxController&&) = delete;

    MaaFwLinuxController(const AsstCallback& callback, Assistant* inst, PlatformType platform_type [[maybe_unused]]) :
        InstHelper(inst),
        m_callback(callback),
        m_loader()
    {
    }

    virtual ~MaaFwLinuxController() override
    {
        if (m_unit) {
            m_loader.destroy(m_unit);
            m_unit = nullptr;
        };
    }

    bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;

    bool inited() const noexcept override { return m_loader.loaded() && m_unit; }

    const std::string& get_uuid() const override;

    size_t get_pipe_data_size() const noexcept override { return { }; }

    size_t get_version() const noexcept override { return { }; }

    bool screencap(cv::Mat& image_payload, bool allow_reconnect = false) override;

    bool start_game(const std::string& client_type [[maybe_unused]]) override
    {
        Log.warn("start_game is not supported on MaaFwLinuxController");
        return false;
    }

    bool stop_game(const std::string& client_type [[maybe_unused]]) override
    {
        Log.warn("stop_game is not supported on MaaFwLinuxController");
        return false;
    }

    bool click(const Point& p) override;

    bool input(const std::string& text [[maybe_unused]]) override
    {
        Log.warn("input is not supported on MaaFwLinuxController");
        return false;
    }

    bool swipe(
        const Point& p1,
        const Point& p2,
        int duration = 0,
        bool extra_swipe = false,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false) override;

    bool inject_input_event(const InputEvent& event) override;

    bool press_esc() override;

    virtual void set_main_screen_recognition(bool on) override { m_main_screen_recognition = on; }

    ControlFeat::Feat support_features() const noexcept override { return ControlFeat::PRECISE_SWIPE; }

    std::pair<int, int> get_screen_res() const noexcept override { return m_screen_size; }

private:
    bool park_cursor();

    static constexpr int DefaultSwipeDelay = 5; // ms

    AsstCallback m_callback;
    MaaFwLinuxControlUnitLoader m_loader;
    MaaFwControlUnitAPI* m_unit = nullptr;

    std::pair<int, int> m_screen_size = { 0, 0 };
    bool m_main_screen_recognition = false;
};

}

#endif
