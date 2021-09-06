#pragma once

#include "AbstractConfiger.h"

#include <string>
#include <unordered_map>
#include <memory>

#include "AsstDef.h"

namespace asst {
	class Configer : public AbstractConfiger
	{
	public:
		virtual ~Configer() = default;

		static Configer& get_instance() {
			static Configer unique_instance;
			return unique_instance;
		}

		//virtual bool load(const std::string& filename) override;

		bool set_param(const std::string& type, const std::string& param, const std::string& value);

		constexpr static int WindowWidthDefault = 1280;
		constexpr static int WindowHeightDefault = 720;
		constexpr static double TemplThresholdDefault = 0.9;
		constexpr static double HistThresholdDefault = 0.9;

		std::string m_version;
		Options m_options;
		std::unordered_map<std::string, std::shared_ptr<TaskInfo>> m_all_tasks_info;
		std::unordered_map<std::string, EmulatorInfo> m_handles;
		std::unordered_map<std::string, std::string> m_recruit_ocr_replace;
		std::unordered_map<std::string, std::string> m_infrast_ocr_replace;

		InfrastOptions m_infrast_options;

	protected:
		Configer() = default;
		Configer(const Configer& rhs) = default;
		Configer(Configer&& rhs) noexcept = default;

		Configer& operator=(const Configer& rhs) = default;
		Configer& operator=(Configer&& rhs) noexcept = default;

		virtual bool parse(json::value&& json) override;
	};
}
