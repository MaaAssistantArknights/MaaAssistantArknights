#pragma once

#include "OcrAbstractTask.h"

namespace asst {
	class OpenRecruitTask : public OcrAbstractTask
	{
	public:
		OpenRecruitTask(AsstCallback callback, void* callback_arg);
		virtual ~OpenRecruitTask() = default;

		virtual bool run() override;
		virtual void set_param(std::vector<int> required_level, bool set_time = true);

	protected:
		std::vector<int> m_required_level;
		bool m_set_time = false;
	};
}