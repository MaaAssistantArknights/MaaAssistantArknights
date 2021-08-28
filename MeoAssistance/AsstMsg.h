#pragma once

#include <ostream>
#include <memory>
#include <unordered_map>
#include <functional>

#include <json.h>

namespace asst {
	enum class AsstMsg {
		/* Error Msg */
		PtrIsNull,
		ImageIsEmpty,
		WindowMinimized,
		InitFaild,
		/* Info Msg */
		TaskStart,
		ImageFindResult,
		ImageMatched,
		TaskMatched,
		ReachedLimit,
		ReadyToSleep,
		EndOfSleep,
		AppendProcessTask,	// 这个消息Assistance会新增任务，外部不需要处理
		AppendTask,			// 这个消息Assistance会新增任务，外部不需要处理
		TaskCompleted,
		PrintWindow,
		TaskStop,
		TaskError,
		/* Open Recruit Msg */
		TextDetected,
		RecruitTagsDetected,
		OcrResultError,
		RecruitSpecialTag,
		RecruitResult,
		/* Infrast Msg*/
		OpersDetected,		// 识别到了干员s
		OpersIdtfResult,	// 干员识别结果（总的）
		InfrastComb			// 当前设置的最优干员组合
	};

	static std::ostream& operator<<(std::ostream& os, const AsstMsg& type)
	{
		static const std::unordered_map<AsstMsg, std::string> _type_name = {
			{AsstMsg::PtrIsNull, "PtrIsNull"},
			{AsstMsg::ImageIsEmpty, "ImageIsEmpty"},
			{AsstMsg::WindowMinimized, "WindowMinimized"},
			{AsstMsg::InitFaild, "InitFaild"},
			{AsstMsg::TaskStart, "TaskStart"},
			{AsstMsg::ImageFindResult, "ImageFindResult"},
			{AsstMsg::ImageMatched, "ImageMatched"},
			{AsstMsg::TaskMatched, "TaskMatched"},
			{AsstMsg::ReachedLimit, "ReachedLimit"},
			{AsstMsg::ReadyToSleep, "ReadyToSleep"},
			{AsstMsg::EndOfSleep, "EndOfSleep"},
			{AsstMsg::AppendProcessTask, "AppendProcessTask"},
			{AsstMsg::TaskCompleted, "TaskCompleted"},
			{AsstMsg::PrintWindow, "PrintWindow"},
			{AsstMsg::TaskError, "TaskError"},
			{AsstMsg::TaskStop, "TaskStop"},
			{AsstMsg::TextDetected, "TextDetected"},
			{AsstMsg::RecruitTagsDetected, "RecruitTagsDetected"},
			{AsstMsg::OcrResultError, "OcrResultError"},
			{AsstMsg::RecruitSpecialTag, "RecruitSpecialTag"},
			{AsstMsg::RecruitResult, "RecruitResult"},
			{AsstMsg::AppendTask, "AppendTask"},
			{AsstMsg::OpersDetected, "OpersDetected"},
			{AsstMsg::OpersIdtfResult, "OpersIdtfResult"},
			{AsstMsg::InfrastComb, "InfrastComb"}
		};
		return os << _type_name.at(type);
	}

	// AsstCallback 消息回调函数
	// 参数：
	// AsstMsg 消息类型
	// const json::value& 消息详情json，每种消息不同，Todo，需要补充个协议文档啥的
	// void* 外部调用者自定义参数，每次回调会带出去，建议传个(void*)this指针进来
	using AsstCallback = std::function<void(AsstMsg, const json::value&, void*)>;
}