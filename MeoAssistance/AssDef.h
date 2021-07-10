#pragma once

#include <utility>

namespace MeoAssistance {
	enum class SimulatorType
	{
		BlueStacks
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