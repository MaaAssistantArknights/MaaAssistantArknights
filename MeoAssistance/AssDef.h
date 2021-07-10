#pragma once

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
}