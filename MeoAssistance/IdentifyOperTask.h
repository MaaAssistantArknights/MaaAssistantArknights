#pragma once

#include "OcrAbstractTask.h"

namespace asst {
	// 识别干员任务，会将识别到的信息写入文件中
	class IdentifyOperTask : public OcrAbstractTask
	{
	public:
		IdentifyOperTask(AsstCallback callback, void* callback_arg);
		virtual ~IdentifyOperTask() = default;

		virtual bool run() override;
		virtual void set_swipe_param(Rect begin, Rect end, int duration) {
			m_swipe_begin = std::move(begin);
			m_swipe_end = std::move(end);
			m_swipe_duration = duration;
		}
		void set_filename(const std::string& filename) {
			m_filename = filename;
		}
	protected:
		constexpr static int SwipeExtraDelay = 0;

		virtual bool keep_swipe(bool reverse = false);
		std::vector<TextArea> detect_opers(const cv::Mat& image,
			std::unordered_map<std::string, std::string>& feature_cond,
			std::unordered_set<std::string>& feature_whatever);
		int detect_elite(const cv::Mat& image, const asst::Rect name_rect);

		Rect m_swipe_begin;
		Rect m_swipe_end;
		int m_swipe_duration = 0;
		bool m_keep_swipe = false;
		std::string m_filename;
	};
}
