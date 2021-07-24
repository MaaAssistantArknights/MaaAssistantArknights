#pragma once

#include <string>
#include <unordered_map>
#include <optional>

#include "AsstDef.h"

namespace asst {

	class Configer
	{
	public:
		Configer() = default;
		~Configer() = default;

		Configer(const Configer& rhs);
		Configer(Configer&& rhs) noexcept;

		bool reload(const std::string& filename);

		bool set_param(const std::string& type, const std::string& param, const std::string& value);
		std::optional<std::string> get_param(const std::string& type, const std::string& param);

		void clear_exec_times();

		Configer& operator=(const Configer& rhs);
		Configer& operator=(Configer&& rhs) noexcept;

		constexpr static double DefaultThreshold = 0.9;
		constexpr static double DefaultCacheThreshold = 0.9;

		std::string m_version;
		Options m_options;
		std::unordered_map<std::string, TaskInfo> m_tasks;
		std::unordered_map<std::string, EmulatorInfo> m_handles;

	private:

	};
}
