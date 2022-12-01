#pragma once

namespace asst::utils
{
    template <typename... Unused>
    constexpr bool always_false = false;
}

// delete instantiation of template with message, when static_assert(false, Message) does not work
#define ASST_STATIC_ASSERT_FALSE(Message, ...) static_assert(::asst::utils::always_false<__VA_ARGS__>, Message);
