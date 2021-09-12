#include "OcrAbstractTask.h"

#include <opencv2/opencv.hpp>

#include "AsstAux.h"
#include "Identify.h"
#include "WinMacro.h"

using namespace asst;

std::vector<TextArea> OcrAbstractTask::ocr_detect(bool use_raw_image)
{
	const cv::Mat& image = m_controller_ptr->get_image(use_raw_image);

	return ocr_detect(image);
}

std::vector<TextArea> OcrAbstractTask::ocr_detect(const cv::Mat& image)
{
	std::vector<TextArea> dst = m_identify_ptr->ocr_detect(image);

	std::vector<json::value> all_text_json_vector;
	for (const TextArea& text_area : dst) {
		all_text_json_vector.emplace_back(text_area.text);
	}
	json::value all_text_json;
	all_text_json["text"] = json::array(all_text_json_vector);
	all_text_json["task_chain"] = m_task_chain;
	m_callback(AsstMsg::TextDetected, all_text_json, m_callback_arg);

	return dst;
}

bool asst::OcrAbstractTask::click_text(const cv::Mat& image, const std::string& text)
{
	for (const TextArea& textarea : ocr_detect(image)) {
		if (textarea.text == text) {
			m_controller_ptr->click(textarea.rect);
			return true;
		}
	}
	return false;
}