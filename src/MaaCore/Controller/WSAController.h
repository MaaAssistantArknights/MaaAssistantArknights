#pragma once

#if defined(_WIN32)

#include "ControllerAPI.h"

#include <queue>
#include <random>

#include "Platform/PlatformFactory.h"

#include <Windows.h>
#include <d3d11.h>

#include <Windows.Graphics.Capture.Interop.h>
#include <windows.graphics.capture.h>
#include <windows.graphics.directx.direct3d11.h>
#include <windows.graphics.directx.direct3d11.interop.h>

#include <winrt/windows.foundation.h>
#include <winrt/windows.graphics.capture.h>
#include <winrt/windows.graphics.directx.direct3d11.h>
#include <winrt/windows.system.h>

#include "Common/AsstMsg.h"
#include "InstHelper.h"

namespace asst
{
    class WSAController : public ControllerAPI, protected InstHelper
    {
    public:
        WSAController(const AsstCallback& callback, Assistant* inst, PlatformType type);
        WSAController(const WSAController&) = delete;
        WSAController(WSAController&&) = delete;
        virtual ~WSAController();

        virtual bool connect([[maybe_unused]] const std::string& adb_path, [[maybe_unused]] const std::string& address,
                             [[maybe_unused]] const std::string& config) override;

        const std::string& get_uuid() const override { return m_uuid; }
        virtual void set_resize_window(bool enable) noexcept override;
        virtual void set_golden_border(bool enable) noexcept override;

        virtual bool inited() const noexcept override { return m_inited; }

        virtual bool screencap(cv::Mat& image_payload, [[maybe_unused]] bool allow_reconnect = false) override;

        virtual bool start_game([[maybe_unused]] const std::string& client_type) override;
        virtual bool stop_game() override { return false; }

        virtual bool click(const Point& p) override;

        virtual bool swipe(const Point& p1, const Point& p2, int duration = 0, bool extra_swipe = false,
                           double slope_in = 1, double slope_out = 1, bool with_pause = false) override;

        virtual bool inject_input_event([[maybe_unused]] const InputEvent& event) override { return false; }

        virtual bool press_esc() override;
        virtual ControlFeat::Feat support_features() const noexcept override
        {
            return ControlFeat::SWIPE_WITH_PAUSE | ControlFeat::PRECISE_SWIPE;
        }

        virtual std::pair<int, int> get_screen_res() const noexcept override { return m_screen_size; }

        WSAController& operator=(const WSAController&) = delete;
        WSAController& operator=(WSAController&&) = delete;

    protected:
        void callback(AsstMsg msg, const json::value& details);

        AsstCallback m_callback;

        std::shared_ptr<asst::PlatformIO> m_platform_io = nullptr;

        std::string m_uuid;
        std::pair<int, int> m_screen_size = { 0, 0 };
        int m_width = 0;
        int m_height = 0;
        bool m_server_started = false;
        bool m_inited = false;
        bool m_resize_window = false;
        bool m_golden_border = false;

        const std::filesystem::path m_wsapath =
            R"(..\Local\Microsoft\WindowsApps\WsaClient.exe /launch wsa://com.hypergryph.arknights)";

        class FrameBuffer
        {
        public:
            FrameBuffer() = delete;
            FrameBuffer(OUT DWORD& last_error);
            ~FrameBuffer();

            bool prepare(bool resize_window, bool golden_border);
            bool start_capture();
            bool end_capture();
            bool restart_capture();
            bool get_once(cv::Mat& payload);

        public:
            std::pair<int, int> pub_size; // 输出大小
            HWND pub_wndwsa = NULL;
            int pub_caption_height = 0;

        private:
            void process_frame(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& framePool,
                               [[maybe_unused]] winrt::Windows::Foundation::IInspectable const& object);

        private:
            winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_frame_pool;
            winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_session;
            winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_item;

            winrt::com_ptr<ID3D11Device> m_native_device;
            winrt::com_ptr<ID3D11DeviceContext> m_context;
            winrt::com_ptr<ID3D11Texture2D> m_staging_texture;
            winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device;

            winrt::Windows::Graphics::SizeInt32 m_size = { 0, 0 }; // 输入大小

            const platform::os_string arknights_classname =
                platform::to_osstring(std::string("com.hypergryph.arknights"));

            std::mutex m_locker;
            cv::Mat m_cache;
            winrt::event_token m_frame_arrived;
            size_t m_pitch = 0;
            cv::Rect m_arknights_roi;
            bool m_golden_border = false;

        } m_wgc; // Windows Graphics Capture member class;
        DWORD m_last_error;

        class Toucher
        {
        public:
            ~Toucher();

            bool init(HWND, int caption_height);

            bool click(int x, int y);
            bool swipe(double sx, double sy, double ex, double ey, int dur, bool pause);
            bool swipe_precisely(double sx, double sy, double ex, double ey, int dur, bool pause);
            void press(int key);
            void wait();

            void run();

        private:
            struct MsgUnit
            {
                int type;
                int x, y;
                MsgUnit(std::initializer_list<int> list)
                {
                    auto beg = list.begin();
                    type = *beg++;
                    x = *beg++;
                    y = *beg++;
                }
            };

            int m_caption_height = 45;

            double linear_interpolate(double& sx, double& sy, double dx, double dy, double time_stamp, double dur_slice,
                                      double n);

        private:
            bool m_inited = false;

            HWND m_wnd = NULL;

            std::thread m_thread;
            std::atomic_bool m_running;
            std::queue<MsgUnit> m_msgs;
            std::vector<double> random_end;

            const size_t max_queue_length = 1024;

        } m_touch;
    };
} // namespace asst

#endif // _WIN32
