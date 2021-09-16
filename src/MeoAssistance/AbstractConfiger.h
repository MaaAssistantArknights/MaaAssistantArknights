#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include <json.h>

#include "AsstDef.h"

namespace json {
	class value;
}

namespace asst {
	class AbstractConfiger
	{
	public:
		virtual ~AbstractConfiger() = default;
		virtual bool load(const std::string& filename);
		virtual bool save();

	protected:
		AbstractConfiger() = default;
		AbstractConfiger(const AbstractConfiger& rhs) = default;
		AbstractConfiger(AbstractConfiger&& rhs) noexcept = default;

		AbstractConfiger& operator=(const AbstractConfiger& rhs) = default;
		AbstractConfiger& operator=(AbstractConfiger&& rhs) noexcept = default;

		virtual bool parse(const json::value& json) = 0;

		std::string m_filename;
		json::value m_raw_json;
	};
}
