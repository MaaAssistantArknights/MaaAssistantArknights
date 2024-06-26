#pragma once

namespace asst
{
class LogControl
{
public:
    static void enable() { g_log_trace_ = true; }

    static void disable() { g_log_trace_ = false; }

    static bool is_enabled() { return g_log_trace_; }

private:
    static bool g_log_trace_;
};
}
