#pragma once

#include <string>
#include <optional>
#include <mutex>

#include "AsstDef.h"
#include "AsstPort.h"

namespace asst {
	struct VersionInfo {
		std::string tag_name;
		std::string html_url;
		std::string down_url;
		std::string author_name;
		std::string created_time;
		std::string body;
	};

	class Updater
	{
		static const std::string GithubReleaseLastestApiUrl;
		static const std::string GithubReleaseApiUrl;
	public:
		~Updater() = default;
		Updater(const Updater&) = delete;
		Updater(Updater&&) = delete;

		static Updater& get_instance();

		bool has_new_version();
		const VersionInfo& get_version_info() const noexcept;

	private:
		Updater() = default;
		std::optional<std::string> request_github_api();

		bool m_has_new_version = false;
		VersionInfo m_lastest_version;
		std::mutex m_mutex;
	};
}