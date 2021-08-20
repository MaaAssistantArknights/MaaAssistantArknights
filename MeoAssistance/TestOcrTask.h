#pragma once

#include "OcrAbstractTask.h"

namespace asst {
	// for debug
	class TestOcrTask : public OcrAbstractTask
	{
	public:
		TestOcrTask(AsstCallback callback, void* callback_arg);
		virtual ~TestOcrTask() = default;

		virtual bool run() override;
		void set_text(std::string text, bool need_click = false)
		{
			m_text_vec.clear();
			m_text_vec.emplace_back(std::move(text));
			m_need_click = need_click;
		}
		void set_text(std::vector<std::string> text_vec, bool need_click = false)
		{
			m_text_vec = std::move(text_vec);
			m_need_click = need_click;
		}
	private:
		std::vector<std::string> m_text_vec;
		bool m_need_click = false;
	};
}