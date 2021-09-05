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
		// 进入下一个宿舍
		bool enter_next_dorm();

		// 进入当前宿舍的干员选择界面
		bool enter_operator_selection();

		// 清空并选择原有的“休息中”干员（相当于把“空闲中”的干员去掉）
		bool clear_and_select_the_resting();
	};
}
