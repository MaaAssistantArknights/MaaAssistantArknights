#pragma once

#ifdef __ANDROID__

#include "../ControllerAPI.h"

#include <functional>
#include <string>

#include "Common/AsstMsg.h"
#include "InstHelper.h"

namespace asst
{

class AndroidController : public ControllerAPI, protected InstHelper
{
public:
    AndroidController(const AsstCallback& callback, Assistant* inst);
    AndroidController(const AndroidController&) = delete;
    AndroidController(AndroidController&&) = delete;
    ~AndroidController() override ;


    virtual bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;
    virtual bool inited() const noexcept override;
    virtual const std::string& get_uuid() const override;
    virtual size_t get_pipe_data_size() const noexcept override;
    virtual size_t get_version() const noexcept override;
    
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
    virtual void back_to_home() noexcept override;

    AndroidController& operator=(const AndroidController&) = delete;
    AndroidController& operator=(AndroidController&&) = delete;

private:
    void callback(AsstMsg msg, const json::value& details);

    bool m_inited = false;
    std::string m_uuid;

    std::pair<int, int> m_screen_resolution = { 1920, 1080 };

    int m_display_id = 0;
    bool m_force_stop = false;

    AsstCallback m_callback = nullptr;
};

} // namespace asst

#endif // __ANDROID__