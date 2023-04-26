#pragma once

#define MAA_NS asst
#define MAA_NS_BEGIN \
    namespace MAA_NS \
    {
#define MAA_NS_END }

/* 步子迈大了 ::vision */
#define MAA_VISION_NS MAA_NS
#define MAA_VISION_NS_BEGIN \
    namespace MAA_VISION_NS \
    {
#define MAA_VISION_NS_END }

#define MAA_UTILS_NS MAA_NS::utils
#define MAA_UTILS_NS_BEGIN \
    namespace MAA_UTILS_NS \
    {
#define MAA_UTILS_NS_END }

MAA_UTILS_NS_BEGIN

template <typename... Unused>
constexpr bool always_false = false;

MAA_UTILS_NS_END

// delete instantiation of template with message, when static_assert(false, Message) does not work
#define ASST_STATIC_ASSERT_FALSE(Message, ...) static_assert(MAA_UTILS_NS::always_false<__VA_ARGS__>, Message);
