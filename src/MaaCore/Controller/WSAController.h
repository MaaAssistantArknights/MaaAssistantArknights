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
            std::atomic_bool m_need_update = false;
            cv::Mat m_front, m_back;
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

            void init(HWND, int caption_height);

            bool click(int x, int y);
            bool swipe(double sx, double sy, double ex, double ey, int dur);
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

            double linear_interpolate(double& sx, double& sy, double dx, double dy, double time_stamp, double dur_slice,
                                      double n);

        private:
            bool m_inited = false;

            HWND m_wnd = NULL;
            int m_caption_height = 45;

            std::thread m_thread;
            std::atomic_bool m_running;
            std::queue<MsgUnit> m_msgs;
            std::vector<double> random_end;

            std::unique_ptr<IOHandler> m_shell;

            const size_t max_queue_length = 1024;

            class VirtualMouse
            {
            public:
                ~VirtualMouse();
                bool inject();
                bool down();
                bool up();

            private:
                void restore();

                static const size_t code_size = 32;
                const unsigned char func_code[1024] =
                    "\x58\x55\x53\x56\x57\x52\x48\x81\xec\x80\x00\x00\x00\x48\x8d\xac\x24\x80\x00\x00\x00\x48\x89\x4d"
                    "\xf4\x48\x89\x45\xf8\x83\x7d\xf4\x01\x75\x2c\x48\x8b\x45\xf8\x48\x83\x78\x60\x01\x75\x12\x48\x81"
                    "\xc4\x80\x00\x00\x00\x5a\x5f\x5e\x5b\x5d\xb8\x01\x80\x00\x00\xc3\x48\x81\xc4\x80\x00\x00\x00\x5a"
                    "\x5f\x5e\x5b\x5d\x31\xc0\xc3\x48\x8b\x45\xf8\x48\x8b\x40\x10\x48\x89\x45\xc8\x48\x8b\x45\xf8\x48"
                    "\x83\xc0\x18\x48\x89\x45\xc0\x48\x8b\x45\xf8\x48\x8b\x00\x48\x89\x45\xb8\x48\x8b\x45\xf8\xff\x50"
                    "\x08\x48\x8b\x55\xb8\x4c\x8b\x45\xc0\x48\x89\xc1\x48\x8b\x45\xc8\x41\xb9\x20\x00\x00\x00\x45\x31"
                    "\xd2\x48\xc7\x44\x24\x20\x00\x00\x00\x00\xff\xd0\x48\x8b\x45\xf8\x48\x8b\x00\x8b\x4d\xf4\xff\xd0"
                    "\x66\x89\x45\xf2\x48\x8b\x45\xf8\x48\x8b\x40\x10\x48\x89\x45\xe0\x48\x8b\x45\xf8\x48\x83\xc0\x38"
                    "\x48\x89\x45\xd8\x48\x8b\x45\xf8\x48\x8b\x00\x48\x89\x45\xd0\x48\x8b\x45\xf8\xff\x50\x08\x48\x8b"
                    "\x55\xd0\x4c\x8b\x45\xd8\x48\x89\xc1\x48\x8b\x45\xe0\x41\xb9\x20\x00\x00\x00\x45\x31\xd2\x48\xc7"
                    "\x44\x24\x20\x00\x00\x00\x00\xff\xd0\x8b\x45\xf2\x48\x81\xc4\x80\x00\x00\x00\x5a\x5f\x5e\x5b\x5d"
                    "\xc3";

                struct
                {
                    LPVOID lpGetKeyState;
                    LPVOID lpGetCurrentProcess;
                    LPVOID lpWriteProcessMemory;
                    unsigned char oldcode[code_size];
                    unsigned char newcode[code_size];
                    LPVOID func_addr;
                    ULONGLONG flag;
                } m_remote;
                const ULONGLONG m_flag_offset = (ULONGLONG)&m_remote.flag - (ULONGLONG)&m_remote;
                unsigned char m_oldcode[code_size];
                unsigned char m_newcode[code_size] =
                    "\x48\xb8\x00\x00\x00\x00\x00\x00\x00\x00\x50\x48\xb8\x00\x00\x00\x00\x00\x00\x00\x00\xff\xe0";

                typedef HANDLE(__stdcall* PFN_CREATEFILE)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD,
                                                          HANDLE);
                typedef int(__stdcall* PFN_MESSAGEBOX)(HWND, LPCWSTR, LPCWSTR, DWORD);
                typedef BOOL(__stdcall* PFN_WRITEPROCESSMEMORY)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*);
                typedef HANDLE(__stdcall* PFN_GETCURRENTPROCESS)(void);
                typedef SHORT(__stdcall* PFN_GetKeyState)(INT);
                typedef VOID(__stdcall* PFN_COPYMEMORY)(LPVOID, LPVOID, SIZE_T);

                HANDLE m_target_process = NULL;
                HMODULE m_kernel32 = NULL;
                HMODULE m_user32 = NULL;

                LPVOID remote_func_addr = nullptr;
                LPVOID remote_para_addr = nullptr;
                LPVOID remote_flag_addr = nullptr;

                bool m_good = false;
            } m_vm;

        } m_touch;
    };
} // namespace asst

#endif // _WIN32
