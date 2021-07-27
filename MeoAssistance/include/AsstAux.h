#pragma once

#include <string>
#include <Windows.h>

namespace asst {

	static std::string GetCurrentDir()
	{
		static std::string cur_dir;
		if (cur_dir.empty()) {
			char exepath_buff[_MAX_PATH] = { 0 };
			::GetModuleFileNameA(NULL, exepath_buff, _MAX_PATH);
			std::string exepath(exepath_buff);
			cur_dir = exepath.substr(0, exepath.find_last_of('\\') + 1);
		}
		return cur_dir;
	}

	static std::string GetResourceDir()
	{
		static std::string res_dir = GetCurrentDir() + "resource\\";
		return res_dir;
	}

	static std::string StringReplaceAll(const std::string& src, const std::string& old_value, const std::string& new_value)
	{
		std::string str = src;
		for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length()) {
			if ((pos = str.find(old_value, pos)) != std::string::npos)
				str.replace(pos, old_value.length(), new_value);
			else
				break;
		}
		return str;
	}

	static std::string GetFormatTimeString()
	{
		SYSTEMTIME curtime;
		GetLocalTime(&curtime);
		char buff[64] = { 0 };
		sprintf_s(buff, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
			curtime.wYear, curtime.wMonth, curtime.wDay,
			curtime.wHour, curtime.wMinute, curtime.wSecond, curtime.wMilliseconds);
		return buff;
	}

	static std::wstring Utf8ToGBK(const std::string& src)
	{
		wchar_t* wbuff = NULL;
		size_t len = (src.size() + 1) * 2;
		wbuff = new wchar_t[len];
		memset(wbuff, 0, len);
		::MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, wbuff, len);
		return wbuff;
	}
}