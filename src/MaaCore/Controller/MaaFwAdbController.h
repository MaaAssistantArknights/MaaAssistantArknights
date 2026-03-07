#pragma once

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

class MaaFwAdbController : public ControllerAPI, private InstHelper, public LibraryHolder<MaaFwAdbController>
{
public:
    MaaFwAdbController(const AsstCallback& callback, Assistant* inst, PlatformType type);
    virtual ~MaaFwAdbController() override;

    MaaFwAdbController(const MaaFwAdbController&) = delete;
    MaaFwAdbController& operator=(const MaaFwAdbController&) = delete;
    MaaFwAdbController(MaaFwAdbController&&) = delete;
    MaaFwAdbController& operator=(MaaFwAdbController&&) = delete;

public:
    virtual bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;
    virtual bool inited() const noexcept override;

    virtual const std::string& get_uuid() const override;

    virtual size_t get_pipe_data_size() const noexcept override { return 114514; }

    virtual size_t get_version() const noexcept override { return 114514; }

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
    std::pair<int, int> m_screen_size = { 0, 0 };

    MaaFwAdbControlUnitAPI* m_unit_handle = nullptr;
    bool init_library();

    AsstCallback m_callback = nullptr;
    void callback(AsstMsg msg, const json::value& details);

    // MaaFramework/source/include/ControlUnit/AdbControlUnitAPI.h
    using GetVersionFunc = const char*();
    using CreateFunc = MaaFwAdbControlUnitAPI*(const char*, const char*, uint64_t, uint64_t, const char*, const char*);
    using DestroyFunc = void(MaaFwAdbControlUnitAPI*);

    std::function<GetVersionFunc> m_get_version_func;
    std::function<CreateFunc> m_create_func;
    std::function<DestroyFunc> m_destroy_func;
};
} // namespace asst
