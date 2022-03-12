#pragma once

#include "AbstractConfiger.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <optional>

#include "AsstTypes.h"

namespace asst
{
    struct AipOcrCfg                        // 百度 OCR API 的配置
    {
        bool enable = false;
        bool accurate = false;              // 是否使用高精度识别
        std::string client_id;
        std::string client_secret;
    };

    struct PenguinReportCfg                 // 企鹅物流数据汇报 的配置
    {
        std::string cmd_format;             // 命令格式
    };

    struct Options
    {
        int task_delay = 0;                  // 任务间延时：越快操作越快，但会增加CPU消耗
        int control_delay_lower = 0;         // 点击随机延时下限：每次点击操作会进行随机延时
        int control_delay_upper = 0;         // 点击随机延时上限：每次点击操作会进行随机延时
        //bool print_window = false;           // 截图功能：开启后每次结算界面会截图到screenshot目录下
        int adb_extra_swipe_dist = 0;        // 额外的滑动距离：adb有bug，同样的参数，偶尔会划得非常远。额外做一个短程滑动，把之前的停下来
        int adb_extra_swipe_duration = -1;   // 额外的滑动持续时间：adb有bug，同样的参数，偶尔会划得非常远。额外做一个短程滑动，把之前的停下来。若小于0，则关闭额外滑动功能
        PenguinReportCfg penguin_report;     // 企鹅数据汇报：每次到结算界面，汇报掉落数据至企鹅数据 https://penguin-stats.cn/
        AipOcrCfg aip_ocr;                   // 百度 OCR API 的配置
    };

    struct AdbCfg
    {
        /* format */
        std::string address_regex;
        std::string display_format;

        /* command */
        std::string devices;
        std::string connect;
        std::string display_id;
        std::string uuid;
        std::string click;
        std::string swipe;
        std::string display;
        std::string screencap_raw_with_gzip;
        std::string screencap_encode;
        std::string release;
    };

    class GeneralConfiger : public AbstractConfiger
    {
    public:
        virtual ~GeneralConfiger() = default;

        const std::string& get_version() const noexcept
        {
            return m_version;
        }
        const Options& get_options() const noexcept
        {
            return m_options;
        }
        Options& get_options()
        {
            return m_options;
        }
        std::optional<AdbCfg> get_adb_cfg(const std::string& name) const
        {
            if (auto iter = m_adb_cfg.find(name);
                iter != m_adb_cfg.cend()) {
                return iter->second;
            }
            else {
                return std::nullopt;
            }
        }

        void set_options(Options opt) noexcept
        {
            m_options = std::move(opt);
        }

    protected:
        virtual bool parse(const json::value& json) override;

        std::string m_version;
        Options m_options;
        std::unordered_map<std::string, AdbCfg> m_adb_cfg;
    };
}
