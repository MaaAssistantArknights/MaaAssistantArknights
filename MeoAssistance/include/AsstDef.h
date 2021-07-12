#pragma once

#include "json_value.h"
#include "json_array.h"

#include <mutex>
#include <process.h>
#include <Windows.h>

namespace asst {
	enum class SimulatorType
	{
		BlueStacks = 0x100
	};
	enum class HandleType
	{
		View = 1,
		Control = 2,
		Window = 4,

		BlueStacksView = 0x100 | 1,
		BlueStacksControl = 0x100 | 2,
		BlueStacksWindow = 0x100 | 4
	};

	struct Point
	{
		Point(int x, int y) : x(x), y(y) {}
		int x = 0;
		int y = 0;
	};

	struct Rect
	{
		Rect(int x, int y, int width, int height)
			: x(x), y(y), width(width), height(height) {}
		Rect operator*(double rhs) const { return { x, y, static_cast<int>(width * rhs), static_cast<int>(height * rhs) }; }
		int x = 0;
		int y = 0;
		int width = 0;
		int height = 0;
	};

	static Rect jsonToRect(const json::array& arr)
	{
		if (arr.size() != 4) {
			return { 0, 0, 0, 0 };
		}
		return Rect(arr[0].as_integer(), arr[1].as_integer(), arr[2].as_integer(), arr[3].as_integer());
	}

	template <typename... Args>
	void DebugPrint(const std::string & level, Args &&... args)
	{
		static std::mutex trace_mutex;
		std::unique_lock<std::mutex> trace_lock(trace_mutex);

		SYSTEMTIME curtime;
		GetLocalTime(&curtime);
		printf("[%04d-%02d-%02d %02d:%02d:%02d.%03d][%s][Px%x][Tx%x] ",
			curtime.wYear, curtime.wMonth, curtime.wDay,
			curtime.wHour, curtime.wMinute, curtime.wSecond, curtime.wMilliseconds,
			level.c_str(), _getpid(), GetCurrentThreadId());
		printf(std::forward<Args>(args)...);
		printf("\n");
	}

	template <typename... Args>
	inline void DebugTrace(Args &&... args)
	{
#ifdef _DEBUG
		DebugPrint("TRC", std::forward<Args>(args)...);
#endif
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
}