#pragma once

#include <string>
#include "json_value.h"

namespace json
{
	static std::string unescape_string(std::string&& str)
	{
		std::string replace_str;
		std::string escape_str = std::move(str);

		for (size_t pos = 0; pos < escape_str.size(); ++pos)
		{
			switch (escape_str[pos]) {
			case '\"':
				replace_str = R"(\")";
				break;
			case '\\':
				replace_str = R"(\\)";
				break;
			case '\b':
				replace_str = R"(\b)";
				break;
			case '\f':
				replace_str = R"(\f)";
				break;
			case '\n':
				replace_str = R"(\n)";
				break;
			case '\r':
				replace_str = R"(\r)";
				break;
			case '\t':
				replace_str = R"(\t)";
				break;
			default:
				continue;
				break;
			}
			escape_str.replace(pos, 1, replace_str);
			++pos;
		}
		return escape_str;
	}

	static std::string unescape_string(const std::string& str)
	{
		return unescape_string(std::string(str));
	}

	static std::string escape_string(std::string&& str)
	{
		std::string escape_str = std::move(str);

		for (size_t pos = 0; pos + 1 < escape_str.size(); ++pos)
		{
			if (escape_str[pos] != '\\') {
				continue;
			}
			std::string replace_str;
			switch (escape_str[pos+1]) {
			case '"':
				replace_str = "\"";
				break;
			case '\\':
				replace_str = "\\";
				break;
			case 'b':
				replace_str = "\b";
				break;
			case 'f':
				replace_str = "\f";
				break;
			case 'n':
				replace_str = "\n";
				break;
			case 'r':
				replace_str = "\r";
				break;
			case 't':
				replace_str = "\r";
				break;
			default:
				return std::string();
				break;
			}
			escape_str.replace(pos, 2, replace_str);
		}
		return escape_str;
	}

	static std::string escape_string(const std::string& str)
	{
		return escape_string(std::string(str));
	}
}