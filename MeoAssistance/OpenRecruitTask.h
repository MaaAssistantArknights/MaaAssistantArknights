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

	// 基建进驻任务
	class InfrastStationTask : public OcrAbstractTask
	{
	public:
		InfrastStationTask(AsstCallback callback, void* callback_arg);
		virtual ~InfrastStationTask() = default;

		virtual bool run() override;
		void set_swipe_param(int delay, const Rect& begin_rect, const Rect& end_rect, int max_times = 20)
		{
			m_swipe_delay = delay;
			m_swipe_begin = begin_rect;
			m_swipe_end = end_rect;
			m_swipe_max_times = max_times;
		}
		void set_facility(const std::string& facility)
		{
			m_facility = facility;
		}
	private:
		std::string m_facility;
		int m_swipe_delay = 0;
		Rect m_swipe_begin;
		Rect m_swipe_end;
		int m_swipe_max_times = 0;
	};
}