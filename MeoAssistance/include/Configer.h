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
		static json::object dataObj;
		static json::object handleObj;

	private:
		Configer() = default;

		static std::string m_curDir;
	};
}
