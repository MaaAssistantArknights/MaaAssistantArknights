#pragma once

#ifdef __ANDROID__

#include <string>
#include <utility>

#include "Common/AsstMsg.h"
#include "ControllerAPI.h"
#include "InstHelper.h"
#include "MaaFwControlUnitInterface.h"
#include "Utils/LibraryHolder.hpp"

namespace asst
{
class Assistant;

class MaaFwAndroidNativeController :
    public ControllerAPI,
    private InstHelper,
    public LibraryHolder<MaaFwAndroidNativeController>
{
public:
    MaaFwAndroidNativeController(const AsstCallback& callback, Assistant* inst);
    virtual ~MaaFwAndroidNativeController() override;

    MaaFwAndroidNativeController(const MaaFwAndroidNativeController&) = delete;
    MaaFwAndroidNativeController& operator=(const MaaFwAndroidNativeController&) = delete;
    MaaFwAndroidNativeController(MaaFwAndroidNativeController&&) = delete;
    MaaFwAndroidNativeController& operator=(MaaFwAndroidNativeController&&) = delete;

public:
    virtual bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;
    virtual bool inited() const noexcept override;

    virtual const std::string& get_uuid() const override;

    virtual size_t get_pipe_data_size() const noexcept override { return 0; }

    virtual size_t get_version() const noexcept override { return 1; }

    virtual bool screencap(cv::Mat& image_payload, bool allow_reconnect = false) override;

    virtual bool start_game(const std::string& client_type) override;
    virtual bool stop_game(const std::string& client_type) override;

    virtual bool click(const Point& p) override;
    virtual bool input(const std::string& text) override;
    virtual bool swipe(
        const Point& p1,
        const Point& p2,
        int duration = 0,
        bool extra_swipe = false,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false) override;

    virtual bool inject_input_event(const InputEvent& event) override;

    virtual bool press_esc() override;
    virtual ControlFeat::Feat support_features() const noexcept override;

    virtual std::pair<int, int> get_screen_res() const noexcept override;

private:
    bool m_inited = false;
    std::string m_uuid;
    std::pair<int, int> m_screen_resolution = { 0, 0 };

    MaaFwAndroidNativeControlUnitAPI* m_unit_handle = nullptr;
    bool init_library();

    AsstCallback m_callback = nullptr;
    void callback(AsstMsg msg, const json::value& details) const;

    // MaaFramework/source/include/MaaControlUnit/AndroidNativeControlUnitAPI.h
    using GetVersionFunc = const char*();
    using CreateFunc = MaaFwAndroidNativeControlUnitAPI*(const char*);
    using DestroyFunc = void(MaaFwAndroidNativeControlUnitAPI*);

    std::function<GetVersionFunc> m_get_version_func;
    std::function<CreateFunc> m_create_func;
    std::function<DestroyFunc> m_destroy_func;
};
} // namespace asst

#endif // __ANDROID__
