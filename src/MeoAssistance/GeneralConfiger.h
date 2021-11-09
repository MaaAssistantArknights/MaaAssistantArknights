#pragma once

#include "AbstractConfiger.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "AsstDef.h"

namespace asst
{
    enum class ConnectType
    {
        Emulator,
        Custom
    };

    struct Options
    {
        ConnectType connect_type;            // 连接类型
        int task_delay = 0;                  // 任务间延时：越快操作越快，但会增加CPU消耗
        int control_delay_lower = 0;         // 点击随机延时下限：每次点击操作会进行随机延时
        int control_delay_upper = 0;         // 点击随机延时上限：每次点击操作会进行随机延时
        bool print_window = false;           // 截图功能：开启后每次结算界面会截图到screenshot目录下
        bool penguin_report = false;         // 企鹅数据汇报：每次到结算界面，是否汇报掉落数据至企鹅数据 https://penguin-stats.cn/
        std::string penguin_report_cmd_line; // 企鹅数据汇报的命令
        std::string penguin_report_server;   // 企鹅数据汇报接口"server"字段，"CN", "US", "JP" and "KR".
        int ocr_gpu_index = -1;              // OcrLite使用GPU编号，-1(使用CPU)/0(使用GPU0)/1(使用GPU1)/...
        int ocr_thread_number = 0;           // OcrLite线程数量
        int adb_extra_swipe_dist = 0;        // 额外的滑动距离：adb有bug，同样的参数，偶尔会划得非常远。额外做一个短程滑动，把之前的停下来
        int adb_extra_swipe_duration = -1;   // 额外的滑动持续时间：adb有bug，同样的参数，偶尔会划得非常远。额外做一个短程滑动，把之前的停下来。若小于0，则关闭额外滑动功能
    };

    class GeneralConfiger : public AbstractConfiger
    {
    public:
        virtual ~GeneralConfiger() = default;

        constexpr static int WindowWidthDefault = 1280;
        constexpr static int WindowHeightDefault = 720;

        const std::string& get_version() const noexcept
        {
            return m_version;
        }
        const Options& get_options() const noexcept
        {
            return m_options;
        }
        const std::unordered_map<std::string, EmulatorInfo>& get_emulators_info() const noexcept
        {
            return m_emulators_info;
        }

        void set_options(Options opt) noexcept
        {
            m_options = std::move(opt);
        }
        void set_emulator_info(std::string name, EmulatorInfo emu)
        {
            m_emulators_info.emplace(std::move(name), std::move(emu));
        }
        void set_emulator_path(std::string name, std::string path)
        {
            m_emulators_info[name].path = path;
        }

    protected:
        virtual bool parse(const json::value& json) override;

        std::string m_version;
        Options m_options;
        std::unordered_map<std::string, EmulatorInfo> m_emulators_info;
    };
}
