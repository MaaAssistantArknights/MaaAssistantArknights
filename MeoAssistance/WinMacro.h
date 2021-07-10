#pragma once

#include <string>
#include <random>
#include <Windows.h>

#include "AssDef.h"

namespace MeoAssistance {
	class WinMacro
	{
	public:
		WinMacro(HandleType type);
		~WinMacro() = default;

		bool findHandle();
		bool resizeWindow(int Width, int Height);
		bool click(Point p);
		bool clickRange(Rect rect);
		bool getImage(Rect rect);
	private:

		HandleType m_handle_type;
		HWND m_handle;
		std::minstd_rand m_rand_engine;
	};
}
