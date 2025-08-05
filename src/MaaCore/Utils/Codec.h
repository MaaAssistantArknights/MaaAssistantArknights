#pragma once

#include <string>
#include <string_view>

namespace asst::utils
{
std::wstring to_u16(std::string_view u8str);
std::string from_u16(std::wstring_view u16str);
}
