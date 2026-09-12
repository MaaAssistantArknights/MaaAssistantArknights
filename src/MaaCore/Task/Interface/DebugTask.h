#pragma once
#include "Common/AsstTypes.h"
#include "Task/InterfaceTask.h"

namespace asst
{
class DebugTask : public InterfaceTask
{
public:
    inline static constexpr std::string_view TaskType = "Debug";

    DebugTask(const AsstCallback& callback, Assistant* inst);
    virtual ~DebugTask() override = default;

    virtual bool run() override;
    virtual bool set_params(const json::value& params) override;

private:
    bool set_params_impl(const json::value& params);
    void test_drops();
    void test_skill_ready();
    void test_battle_image();
    void test_match_template();

    // 以下为离线图片评估入口：不连设备，对本地图片跑与线上一致的识别链路，
    // 结果通过 SubTaskExtraInfo 回调（what = "DebugImageTest"）与日志输出。
    // 配套 python 驱动 tools/maa_core_eval.py（可 import 或 CLI），测试本地图片用它即可
    bool image_test_report();    // 每张图 × 每个任务独立评估命中情况
    bool image_test_pipeline();  // 每张图按任务列表跑一次首命中，附带命中任务的 next 列表
    bool image_test_ocr();       // 返回 OCR 原始识别文本（不套任务的 ocrReplace 与 expected 过滤）
    bool image_test_templ();     // 每张图 × 裸模板文件匹配（物品图标等非任务模板），报最佳得分

    std::string m_image_test_mode; // 空 = 未启用（run 空跑，保持旧行为）；report / pipeline / ocr / templ
    std::vector<std::string> m_eval_images;
    std::vector<std::string> m_eval_tasks;
    std::vector<std::string> m_eval_templates;
    std::string m_eval_templ_task; // templ 模式的 Matcher 配置来源任务名（mask/method/roi 等取自它）
    Rect m_eval_roi;
    double m_eval_threshold = TemplThresholdDefault;
    std::optional<std::pair<int, int>> m_eval_resize; // 1280x720 归一后的二级缩放尺寸（templ 模式）

    // 读图失败等单图错误也要发对应的结果条目，否则调用方按图取结果会缺项
    void emit_eval_error(const std::string& mode, const std::string& image_path, const std::string& error);
};
}
