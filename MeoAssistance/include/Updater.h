#pragma once

#include <string>
#include <optional>
#include <mutex>

namespace asst {
	struct __declspec(dllexport) VersionInfo {
		std::string tag_name;
		std::string html_url;
		std::string down_url;
		std::string author_name;
		std::string created_time;
		std::string body;
	};

	class __declspec(dllexport) Updater
	{
		static const std::string GithubReleaseLastestApiUrl;
		static const std::string GithubReleaseApiUrl;
	public:
		~Updater() = default;

		static Updater& get_instance();

		bool has_new_version();
		const VersionInfo & get_version_info() const noexcept;

	private:
		Updater() = default;
		std::optional<std::string> request_github_api();

		bool m_has_new_version = false;
		VersionInfo m_lastest_version;
		std::mutex m_mutex;
	};
}