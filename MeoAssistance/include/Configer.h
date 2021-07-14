#pragma once

#include <string>
#include "json_object.h"

namespace asst {
	class Configer
	{
	public:
		~Configer() = default;

		static bool reload();
		static std::string getCurDir();
		static std::string getResDir();
		static json::object tasksJson;
		static json::object handleJson;
		static json::object optionsJson;
	private:
		Configer() = default;

		static std::string m_curDir;
	};
}
