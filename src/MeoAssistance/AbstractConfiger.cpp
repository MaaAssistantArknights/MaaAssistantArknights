#include "AbstractConfiger.h"

#include <memory>
#include <sstream>
#include <fstream>

#include <json.h>

#include "Logger.hpp"

bool asst::AbstractConfiger::load(const std::string& filename)
{
	DebugTraceFunction;
	DebugTrace("Configer::load | ", filename);

	std::ifstream ifs(filename, std::ios::in);
	if (!ifs.is_open()) {
		return false;
	}
	std::stringstream iss;
	iss << ifs.rdbuf();
	ifs.close();
	std::string content(iss.str());

	auto&& ret = json::parser::parse(content);
	if (!ret) {
		DebugTrace("parse error", content);
		return false;
	}

	json::value root = std::move(ret.value());

	try {
		parse(root);
	}
	catch (json::exception& e) {
		DebugTraceError("Load config json error!", e.what());
		return false;
	}
	m_filename = filename;
	m_raw_json = std::move(root);

	DebugTrace("Load config succeed");
	return true;
}

bool asst::AbstractConfiger::save()
{
	std::ofstream ofs(m_filename, std::ios::out);
	if (!ofs.is_open()) {
		return false;
	}
	ofs << m_raw_json.format();
	ofs.close();

	return true;
}
