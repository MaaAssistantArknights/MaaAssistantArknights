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

	struct PointRange
	{
		PointRange(int left, int top, int right, int bottom)
			: left(left), top(top), right(right), bottom(bottom) {}
		int left = 0;
		int top = 0;
		int right = 0;
		int bottom = 0;
	};
}