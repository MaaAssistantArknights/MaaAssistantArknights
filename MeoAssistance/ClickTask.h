#pragma once

#include "AsstDef.h"
#include "AbstractTask.h"

namespace asst {
	class ClickTask : public AbstractTask
	{
	public:
		ClickTask(AsstCallback callback, void* callback_arg);
		virtual ~ClickTask() = default;

		virtual bool run() override;
		void set_rect(asst::Rect rect) { m_rect = std::move(rect); };
	protected:
		asst::Rect m_rect;
	};
}