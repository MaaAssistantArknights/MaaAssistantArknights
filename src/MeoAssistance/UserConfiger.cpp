#include "UserConfiger.h"

#include <fstream>

#include <json.h>

#include "Configer.h"

bool asst::UserConfiger::load(const std::string& filename)
{
	AbstractConfiger::load(filename);
	m_filename = filename;
	return true;
}

bool asst::UserConfiger::set_emulator_path(const std::string& name, const std::string& path)
{
	m_raw_json["emulatorPath"][name] = path;
	return save();
}

bool asst::UserConfiger::parse(const json::value& json)
{
	if (json.exist("emulatorPath")) {
		for (const auto& [name, path] : json.at("emulatorPath").as_object()) {
			Configer::get_instance().m_handles[name].path = path.as_string();
		}
	}
	m_raw_json = json;

	return true;
}

bool asst::UserConfiger::save()
{
	std::ofstream ofs(m_filename, std::ios::out);
	if (!ofs.is_open()) {
		return false;
	}
	ofs << m_raw_json.format();
	ofs.close();

	return true;
}