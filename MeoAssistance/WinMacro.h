#pragma once

#include <string>
#include <random>
#include <Windows.h>

#include "AssDef.h"

namespace MeoAssistance {
	class WinMacro
	{
	public:
		WinMacro(SimulatorType type);
		~WinMacro() = default;

		bool findHandle();
		void click(Point p);
		void clickRange(PointRange pr);
	private:

		SimulatorType m_simulator_type;
		HWND m_view_handle;
		HWND m_control_handle;
		std::minstd_rand m_rand_engine;
	};
}
