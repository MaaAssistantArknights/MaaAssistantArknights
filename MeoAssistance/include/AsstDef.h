#pragma once

#include <mutex>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <process.h>
#include <Windows.h>

namespace asst {

	const static std::string Version = "release.beta.01.2";

	/************ Type ************/

	enum class HandleType
	{
		Window = 1,
		View = 2,
		Control = 4
	};

	struct Point
	{
		Point() = default;
		Point(int x, int y) : x(x), y(y) {}
		int x = 0;
		int y = 0;
	};

	struct Rect
	{
		Rect() = default;
		Rect(int x, int y, int width, int height)
			: x(x), y(y), width(width), height(height) {}
		Rect operator*(double rhs) const
		{
			return { x, y, static_cast<int>(width * rhs), static_cast<int>(height * rhs) };
		}
		Rect center_zoom(double scale)
		{
			int half_width_scale = static_cast<int>(width * (1 - scale) / 2);
			int half_hight_scale = static_cast<int>(height * (1 - scale) / 2);
			return { x + half_width_scale, y + half_hight_scale, width - half_width_scale,  height - half_hight_scale };
		}
		int x = 0;
		int y = 0;
		int width = 0;
		int height = 0;
	};

	/************ Static Function ************/

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

	static std::string replace_all_distinct(const std::string& src, const std::string& old_value, const std::string& new_value)
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

	/************ Print and Log Trace ************/

	inline void StreamArgs(std::ostream& os)
	{
		os << std::endl;
	}
	template <typename T, typename... Args>
	inline void StreamArgs(std::ostream& os, T&& first, Args && ...rest)
	{
		os << first << " ";
		StreamArgs(os, std::forward<Args>(rest)...);
	}
	template <typename... Args>
	void DebugPrint(const std::string& level, Args &&... args)
	{
		static std::mutex trace_mutex;
		std::unique_lock<std::mutex> trace_lock(trace_mutex);

		SYSTEMTIME curtime;
		GetLocalTime(&curtime);
		char buff[64] = { 0 };
		sprintf_s(buff, "[%04d-%02d-%02d %02d:%02d:%02d.%03d][%s][Px%x][Tx%x]",
			curtime.wYear, curtime.wMonth, curtime.wDay,
			curtime.wHour, curtime.wMinute, curtime.wSecond, curtime.wMilliseconds,
			level.c_str(), _getpid(), GetCurrentThreadId());

		if (level == "ERR" || level == "INF" 
#ifdef _DEBUG
			|| level == "TRC"
#endif
			) {
			StreamArgs(std::cout, buff, std::forward<Args>(args)...);
		}
		std::ofstream out_stream(GetCurrentDir() + "asst.log", std::ios::out | std::ios::app);
		StreamArgs(out_stream, buff, std::forward<Args>(args)...);
	}

	template <typename... Args>
	inline void DebugTrace(Args &&... args)
	{
		//#ifdef _DEBUG
		DebugPrint("TRC", std::forward<Args>(args)...);
		//#endif
	}
	template <typename... Args>
	inline void DebugTraceInfo(Args &&... args)
	{
		DebugPrint("INF", std::forward<Args>(args)...);
	}
	template <typename... Args>
	inline void DebugTraceError(Args &&... args)
	{
		DebugPrint("ERR", std::forward<Args>(args)...);
	}

	class DebugTraceFuncAux {
	public:
		DebugTraceFuncAux(const std::string& func_name)
			: m_func_name(func_name),
			m_start_time(std::chrono::system_clock::now())
		{
			DebugTrace(m_func_name, " | enter");
		}
		~DebugTraceFuncAux()
		{
			auto duration = std::chrono::system_clock::now() - m_start_time;
			DebugTrace(m_func_name, " | leave,",
				std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(), "ms");
		}
	private:
		std::string m_func_name;
		std::chrono::time_point<std::chrono::system_clock> m_start_time;
	};

#define DebugTraceFunction DebugTraceFuncAux _func_aux(__FUNCTION__)

}