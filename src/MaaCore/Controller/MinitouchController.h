#pragma once

#include "AdbController.h"

#include "Config/GeneralConfig.h"

#include <thread>

namespace asst
{
class MinitouchController : public AdbController
{
public:
    MinitouchController(const AsstCallback& callback, Assistant* inst, PlatformType type) :
        AdbController(callback, inst, type)
    {
    }

    MinitouchController(const MinitouchController&) = delete;
    MinitouchController(MinitouchController&&) = delete;
    virtual ~MinitouchController();

    virtual bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;

    virtual void set_swipe_with_pause(bool enable) noexcept override;

    virtual bool click(const Point& p) override;

    virtual bool swipe(
        const Point& p1,
        const Point& p2,
        int duration = 0,
        bool extra_swipe = false,
        double slope_in = 1,
        double slope_out = 1,
        bool with_pause = false) override;

    virtual bool inject_input_event(const InputEvent& event) override;

    virtual ControlFeat::Feat support_features() const noexcept override;

    MinitouchController& operator=(const MinitouchController&) = delete;
    MinitouchController& operator=(MinitouchController&&) = delete;

    virtual void back_to_home() noexcept override;

protected:
    virtual std::optional<std::string> reconnect(const std::string& cmd, int64_t timeout, bool recv_by_socket) override;

    bool call_and_hup_minitouch();

    bool probe_minitouch(const AdbCfg& adb_cfg, std::function<std::string(const std::string&)> cmd_replace);

    bool input_to_minitouch(const std::string& cmd);
    void release_minitouch(bool force = false);

    bool use_swipe_with_pause() const noexcept;

    virtual void clear_info() noexcept override;

    bool m_minitouch_available = false;
    bool m_use_maa_touch = false;
    bool m_swipe_with_pause_enabled = false;

    std::shared_ptr<IOHandler> m_minitouch_handler = nullptr;

    class Minitoucher;
    std::unique_ptr<Minitoucher> m_minitoucher = nullptr;

    struct MinitouchProps
    {
        int max_contacts = 0;
        int max_x = 0;
        int max_y = 0;
        int max_pressure = 0;

        double x_scaling = 0;
        double y_scaling = 0;
        int orientation = 0;
    } m_minitouch_props;

    class Minitoucher
    {
    public:
        using InputFunc = std::function<bool(const std::string&)>;

    public:
        static constexpr int DefaultClickDelay = 50;
        static constexpr int DefaultSwipeDelay = 2;
        static constexpr int ExtraDelay = 0;

        Minitoucher(InputFunc func, const MinitouchProps& props) :
            m_input_func(func),
            m_props(props)
        {
        }

        ~Minitoucher() = default;

        // nodiscard! for false return value may indicating *this got replaced in m_input_func
        [[nodiscard]] bool reset() { return m_input_func(reset_cmd()); }

        [[nodiscard]] bool commit() { return m_input_func(commit_cmd()); }

        [[nodiscard]] bool down(int x, int y, int wait_ms = DefaultClickDelay, bool with_commit = true, int contact = 0)
        {
            return m_input_func(down_cmd(x, y, wait_ms, with_commit, contact));
        }

        [[nodiscard]] bool move(int x, int y, int wait_ms = DefaultSwipeDelay, bool with_commit = true, int contact = 0)
        {
            return m_input_func(move_cmd(x, y, wait_ms, with_commit, contact));
        }

        [[nodiscard]] bool up(int wait_ms = DefaultClickDelay, bool with_commit = true, int contact = 0)
        {
            return m_input_func(up_cmd(wait_ms, with_commit, contact));
        }

        [[nodiscard]] bool key_down(int key_code, int wait_ms = DefaultClickDelay, bool with_commit = true)
        {
            return m_input_func(key_down_cmd(key_code, wait_ms, with_commit));
        }

        [[nodiscard]] bool key_up(int key_code, int wait_ms = DefaultClickDelay, bool with_commit = true)
        {
            return m_input_func(key_up_cmd(key_code, wait_ms, with_commit));
        }

        [[nodiscard]] bool wait(int ms) { return m_input_func(wait_cmd(ms)); }

        void clear() noexcept { m_wait_ms_count = 0; }

        void extra_sleep() { sleep(); }

    private:
        [[nodiscard]] std::string reset_cmd() const noexcept { return "r\n"; }

        [[nodiscard]] std::string commit_cmd() const noexcept { return "c\n"; }
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
        [[nodiscard]] std::string
            down_cmd(int x, int y, int wait_ms = DefaultClickDelay, bool with_commit = true, int contact = 0)
        {
            // mac 编不过
            // std::string str = std::format("d {} {} {} {}\n", contact, x, y, pressure);

            char buff[64] = { 0 };
            auto [c_x, c_y] = scale(x, y);
            sprintf(buff, "d %d %d %d %d\n", contact, c_x, c_y, m_props.max_pressure);
            std::string str = buff;

            if (with_commit) {
                str += commit_cmd();
            }
            if (wait_ms) {
                str += wait_cmd(wait_ms);
            }
            return str;
        }

        [[nodiscard]] std::string
            move_cmd(int x, int y, int wait_ms = DefaultSwipeDelay, bool with_commit = true, int contact = 0)
        {
            char buff[64] = { 0 };
            auto [c_x, c_y] = scale(x, y);
            sprintf(buff, "m %d %d %d %d\n", contact, c_x, c_y, m_props.max_pressure);
            std::string str = buff;

            if (with_commit) {
                str += commit_cmd();
            }
            if (wait_ms) {
                str += wait_cmd(wait_ms);
            }
            return str;
        }

        [[nodiscard]] std::string up_cmd(int wait_ms = DefaultClickDelay, bool with_commit = true, int contact = 0)
        {
            char buff[16] = { 0 };
            sprintf(buff, "u %d\n", contact);
            std::string str = buff;

            if (with_commit) {
                str += commit_cmd();
            }
            if (wait_ms) {
                str += wait_cmd(wait_ms);
            }
            return str;
        }

        [[nodiscard]] std::string key_down_cmd(int key_code, int wait_ms = DefaultClickDelay, bool with_commit = true)
        {
            char buff[64] = { 0 };
            sprintf(buff, "k %d d\n", key_code);
            std::string str = buff;

            if (with_commit) {
                str += commit_cmd();
            }
            if (wait_ms) {
                str += wait_cmd(wait_ms);
            }
            return str;
        }

        [[nodiscard]] std::string key_up_cmd(int key_code, int wait_ms = DefaultClickDelay, bool with_commit = true)
        {
            char buff[64] = { 0 };
            sprintf(buff, "k %d u\n", key_code);
            std::string str = buff;

            if (with_commit) {
                str += commit_cmd();
            }
            if (wait_ms) {
                str += wait_cmd(wait_ms);
            }
            return str;
        }

        [[nodiscard]] std::string wait_cmd(int ms)
        {
            m_wait_ms_count += ms;
            char buff[16] = { 0 };
            sprintf(buff, "w %d\n", ms);
            return buff;
        }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        void sleep()
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(m_wait_ms_count * 1ms);
            m_wait_ms_count = 0;
        }

    private:
        Point scale(int x, int y) const noexcept
        {
            switch (m_props.orientation) {
            case 0:
            default:
                return {
                    static_cast<int>(x * m_props.x_scaling),
                    static_cast<int>(y * m_props.y_scaling),
                };
            case 1:
                return {
                    m_props.max_y - static_cast<int>(y * m_props.y_scaling),
                    static_cast<int>(x * m_props.x_scaling),
                };
            case 2:
                return {
                    m_props.max_x - static_cast<int>(x * m_props.x_scaling),
                    m_props.max_y - static_cast<int>(y * m_props.y_scaling),
                };
            case 3:
                return {
                    static_cast<int>(y * m_props.y_scaling),
                    m_props.max_x - static_cast<int>(x * m_props.x_scaling),
                };
            }
        }

        const std::function<bool(const std::string&)> m_input_func = nullptr;
        const MinitouchProps& m_props;
        int m_wait_ms_count = ExtraDelay;
    };
};
} // namespace asst
