#pragma once

#include "MinitouchController.h"

namespace asst
{
class MaatouchController : public MinitouchController
{
public:
    MaatouchController(const AsstCallback& callback, Assistant* inst, PlatformType type) :
        MinitouchController(callback, inst, type)
    {
        m_use_maa_touch = true;
    }

    MaatouchController(const MaatouchController&) = delete;
    MaatouchController(MaatouchController&&) = delete;
    virtual ~MaatouchController() = default;

    virtual bool inject_input_event(const InputEvent& event) override;

    virtual bool press_esc() override;

protected:
    virtual bool call_and_hup_minitouch() override;

    class Maatoucher : public Minitoucher
    {
    public:
        Maatoucher(InputFunc func, const MinitouchProps& props) :
            Minitoucher(func, props)
        {
        }

        [[nodiscard]] bool key_down(int key_code, int wait_ms = DefaultClickDelay, bool with_commit = true)
        {
            return m_input_func(key_down_cmd(key_code, wait_ms, with_commit));
        }

        [[nodiscard]] bool key_up(int key_code, int wait_ms = DefaultClickDelay, bool with_commit = true)
        {
            return m_input_func(key_up_cmd(key_code, wait_ms, with_commit));
        }

    protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
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

#ifdef _MSC_VER
#pragma warning(pop)
#endif
    };
};
} // namespace asst
