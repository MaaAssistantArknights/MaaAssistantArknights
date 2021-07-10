#pragma once

#include "json_value.h"
#include "json_array.h"

namespace MeoAssistance {
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
}