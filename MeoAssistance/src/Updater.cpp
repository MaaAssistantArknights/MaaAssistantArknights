#include "Updater.h"

#include <stdio.h>
#include <Windows.h>
#include <WinInet.h>

#include "json.h"
#include "AsstDef.h"

using namespace asst;

const std::string Updater::GithubReleaseLastestApiUrl = "https://api.github.com/repos/MistEO/MeoAssistance/releases/latest";
const std::string Updater::GithubReleaseApiUrl = "https://api.github.com/repos/MistEO/MeoAssistance/releases/latest";

Updater& Updater::instance()
{
	static Updater unique_instance;
	return unique_instance;
}

bool Updater::has_new_version()
{
	auto req_ret = request_github_api();
	if (!req_ret) {
		DebugTraceInfo("Requeset Error");
		return false;
	}
	auto parse_ret = json::parser::parse(req_ret.value());
	if (!parse_ret) {
		DebugTraceInfo("Parse Error");
		return false;
	}

	json::value root = std::move(parse_ret).value();
	try {
		m_lastest_version.tag_name = root["tag_name"].as_string();
		m_lastest_version.html_url = root["html_url"].as_string();
		m_lastest_version.author_name = root["author"]["login"].as_string();
		m_lastest_version.created_time = root["created_at"].as_string();
		m_lastest_version.body = root["body"].as_string();

		auto assets = root["assets"].as_array();
		if (assets.size() == 1) {
			m_lastest_version.down_url = assets[0]["browser_download_url"].as_string();
		}
	}
	catch (json::exception& exp) {
		DebugTraceError("Json Error", exp.what());
		return false;
	}

	if (m_lastest_version.tag_name > Version) {
		m_has_new_version = true;
		return true;
	}
	else {
		return false;
	}
}

const VersionInfo & Updater::get_version_info() const noexcept
{
	if (!m_has_new_version) {
		return VersionInfo();
	}
	return m_lastest_version;
}

std::optional<std::string> Updater::request_github_api()
{
	HINTERNET h_session = ::InternetOpenA("GithubRelease", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (h_session == NULL) {
		return std::nullopt;
	}
	HINTERNET h_http = ::InternetOpenUrlA(h_session, GithubReleaseLastestApiUrl.c_str(), NULL, 0, INTERNET_FLAG_DONT_CACHE, 0);
	if (h_http == NULL) {
		return std::nullopt;
	}
	std::string response;
	size_t buff_size = 4096;
	char *buffer = new char[buff_size];
	DWORD number = 1;
	while (number > 0) {
		memset(buffer, 0, buff_size);
		InternetReadFile(h_http, buffer, buff_size, &number);
		if (number != 0) {
			response += std::string(buffer, number);
		}
	}
	delete[] buffer;
	buffer = NULL;
	InternetCloseHandle(h_http);
	h_http = NULL;
	InternetCloseHandle(h_session);
	h_session = NULL;

	return response;
}