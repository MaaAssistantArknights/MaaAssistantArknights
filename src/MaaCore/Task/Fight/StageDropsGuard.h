#pragma once

#include <functional>
#include <utility>

namespace asst
{
// Opt-in for consumers that require every result to update a verified ledger.
// Invoked synchronously after the receipt callback, never by upload threads.
class StageDropsGuard
{
public:
    void set(std::function<bool()> validate) { m_validate = std::move(validate); }

    bool allows_next_battle(bool recognition_ok) const
    {
        // Keep ordinary Fight behavior unchanged when no consumer opted in.
        return !m_validate || (recognition_ok && m_validate());
    }

private:
    std::function<bool()> m_validate;
};
}
