#pragma once

#include "AbstractTask.h"
#include "AsstDef.h"

namespace asst {
	class SwipeTask : public AbstractTask
	{
		SwipeTask(AsstCallback callback, void* callback_arg);
		virtual ~SwipeTask() = default;

		virtual bool run() override;

		virtual void set_swipe_param(Rect begin, Rect end, int duration) {
			m_swipe_begin = std::move(begin);
			m_swipe_end = std::move(end);
			m_swipe_duration = duration;
		}
		virtual void set_reverse(bool reverse) {
			m_reverse = reverse;
		}
	protected:
		constexpr static int SwipeExtraDelay = 100;

		virtual bool swipe();

		Rect m_swipe_begin;
		Rect m_swipe_end;
		int m_swipe_duration = 0;
		bool m_reverse = false;
	};
}
