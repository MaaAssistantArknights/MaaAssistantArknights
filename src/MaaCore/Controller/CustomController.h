#pragma once

#include "ControllerAPI.h"

#include "AsstCustom.h"

#include "Common/AsstMsg.h"
#include "InstHelper.h"

namespace asst
{
    class CustomController : public ControllerAPI, protected InstHelper
    {
    public:
        CustomController(const AsstCallback& callback, Assistant* inst, std::shared_ptr<AsstCustomController> custom);
        virtual ~CustomController() override {};

        virtual bool connect(const std::string& adb_path, const std::string& address,
                             const std::string& config) override;
        virtual bool inited() const noexcept override;
        virtual void set_swipe_with_pause(bool enable) noexcept override;

        virtual const std::string& get_uuid() const override;

        virtual bool screencap(cv::Mat& image_payload, bool allow_reconnect = false) override;

        virtual bool start_game(const std::string& client_type) override;
        virtual bool stop_game() override;

        virtual bool click(const Point& p) override;

        virtual bool swipe(const Point& p1, const Point& p2, int duration = 0, bool extra_swipe = false,
                           double slope_in = 1, double slope_out = 1, bool with_pause = false) override;

        virtual bool inject_input_event(const InputEvent& event) override;

        virtual bool press_esc() override;
        virtual ControlFeat::Feat support_features() const noexcept override;

        virtual std::pair<int, int> get_screen_res() const noexcept override;

    private:
        void callback(AsstMsg msg, const json::value& details);
        AsstCallback m_callback;
        std::shared_ptr<AsstCustomController> m_custom = nullptr;

        std::string m_uuid;
        std::pair<int, int> m_screen_res = { 0, 0 };
    };
}