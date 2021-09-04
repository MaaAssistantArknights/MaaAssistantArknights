#pragma once

#include "OcrAbstractTask.h"

namespace asst {
	class InfrastDormTask : public OcrAbstractTask
	{
	public:
		InfrastDormTask(AsstCallback callback, void* callback_arg);
		virtual ~InfrastDormTask() = default;

		virtual bool run() override;

	protected:
		// 进入宿舍，index为从上到下的编号
		bool enter_dorm(int index = 0);
	};
}
