#include "TestOcrTask.h"

#include <opencv2/opencv.hpp>

#include "AsstAux.h"
#include "WinMacro.h"
#include "Identify.h"
#include "Configer.h"
#include "InfrastConfiger.h"

using namespace asst;

TestOcrTask::TestOcrTask(AsstCallback callback, void* callback_arg)
	: OcrAbstractTask(callback, callback_arg)
{
	m_task_type = TaskType::TaskTypeRecognition & TaskType::TaskTypeClick;
}

bool TestOcrTask::run()
{
	if (m_view_ptr == NULL
		|| m_identify_ptr == NULL)
	{
		m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}

	auto detect_foo = [&](const cv::Mat& image) -> std::vector<TextArea> {
		std::vector<TextArea> all_text_area = ocr_detect(image);
		/* 过滤出所有制造站中的干员名 */
		std::vector<TextArea> cur_name_textarea = text_search(
			all_text_area,
			InfrastConfiger::get_instance().m_all_opers_name,
			Configer::get_instance().m_infrast_ocr_replace);
		return cur_name_textarea;
	};

	const cv::Mat& image = get_format_image(true);
	auto cur_name_textarea = detect_foo(image);

	// for debug
	cv::Mat elite1 = cv::imread(GetResourceDir() + "operators\\Elite1.png");
	cv::Mat elite2 = cv::imread(GetResourceDir() + "operators\\Elite2.png");
	for (const TextArea& textarea : cur_name_textarea)
	{
		cv::Rect elite_rect;
		elite_rect.x = textarea.rect.x - 200;
		elite_rect.y = textarea.rect.y - 200;
		elite_rect.width = 100;
		elite_rect.height = 150;
		auto&& [score1, point1] = m_identify_ptr->match_template(image(elite_rect), elite1);
		auto&& [score2, point2] = m_identify_ptr->match_template(image(elite_rect), elite2);

		if (score1 > score2 && score1 > 0.7) {
			std::cout << Utf8ToGbk(textarea.text) << "，精一， elite1: " << score1 << ", eilte2: " << score2 << std::endl;
		}
		else if (score2 > score1 && score2 > 0.7) {
			std::cout << Utf8ToGbk(textarea.text) << "，精二， elite1: " << score1 << ", eilte2: " << score2 << std::endl;
		}
		else {
			std::cout << Utf8ToGbk(textarea.text) << "，未精英化， elite1: " << score1 << ", eilte2: " << score2 << std::endl;
		}

		//m_identify_ptr->feature_match(image(elite_rect), "Elite1");
		//m_identify_ptr->feature_match(image(elite_rect), "Elite2");
	}

	return true;
}