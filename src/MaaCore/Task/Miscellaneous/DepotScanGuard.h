#pragma once

#include <cstddef>
#include <limits>
#include <optional>

namespace asst
{
// Opt-in quota scan policy. Legacy Depot display is not made stricter.
// A failed page is not EOF. Completion needs a successfully analyzed repeat
// of the last ordered page, not merely any quantities collected earlier.
class DepotScanGuard
{
public:
    enum class Decision
    {
        Continue,
        Complete,
        Reject
    };
    static constexpr size_t MaxPages = 64;

    Decision observe(bool analyzed, size_t position, bool page_unchanged = false)
    {
        if (m_terminal || ++m_pages > MaxPages || !analyzed || position == std::numeric_limits<size_t>::max()) {
            m_terminal = true;
            return Decision::Reject;
        }
        if (m_previous && position == *m_previous) {
            m_terminal = true;
            // An equal last matched ID on a different image may only be a
            // truncated analysis. It is not evidence of the end of the list.
            return page_unchanged ? Decision::Complete : Decision::Reject;
        }
        if (m_previous && position < *m_previous) {
            m_terminal = true;
            return Decision::Reject;
        }
        m_previous = position;
        return Decision::Continue;
    }

private:
    size_t m_pages = 0;
    std::optional<size_t> m_previous;
    bool m_terminal = false;
};
}
