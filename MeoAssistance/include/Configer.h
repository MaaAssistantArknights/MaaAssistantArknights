#pragma once

#include <string>
#include <unordered_map>
#include <optional>

#include "AsstDef.h"

namespace asst {

	class Configer
	{
	public:
		~Configer() = default;

		static Configer& get_instance() {
			static Configer unique_instance;
			return unique_instance;
		}

		bool load(const std::string& filename);

		bool set_param(const std::string& type, const std::string& param, const std::string& value);

		Configer& operator=(const Configer& rhs) = default;
		Configer& operator=(Configer&& rhs) noexcept = default;

		constexpr static int DefaultWindowWidth = 1280;
		constexpr static int DefaultWindowHeight = 720;
		constexpr static double Defaulttempl_threshold = 0.9;
		constexpr static double DefaultCachetempl_threshold = 0.9;

		std::string m_version;
		Options m_options;
		std::unordered_map<std::string, TaskInfo> m_all_tasks_info;
		std::unordered_map<std::string, EmulatorInfo> m_handles;
		std::unordered_map<std::string, std::string> m_ocr_replace;

	private:
		Configer() = default;
		Configer(const Configer & rhs) = default;
		Configer(Configer && rhs) noexcept = default;

		bool _load(const std::string& filename);
	};
}
