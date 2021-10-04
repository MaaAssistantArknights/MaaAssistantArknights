#pragma once

#include "AbstractConfiger.h"

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "AsstDef.h"

namespace asst {
    class InfrastConfiger : public AbstractConfiger
    {
    public:
        virtual ~InfrastConfiger() = default;

        static InfrastConfiger& get_instance() {
            static InfrastConfiger unique_instance;
            return unique_instance;
        }

        std::unordered_set<std::string> m_all_opers_name;				// 所有干员的名字
        // 各个设施内的可能干员组合，注意值是vector，是有序的
        std::unordered_map<std::string, std::vector<OperInfrastComb>> m_infrast_combs;
        // 各个设施的识别结束标记，识别到这个名字就停止继续识别了，一般放列表中最后几个人名
        std::unordered_map<std::string, std::vector<std::string>> m_infrast_end_flag;

    protected:
        InfrastConfiger() = default;
        InfrastConfiger(const InfrastConfiger& rhs) = default;
        InfrastConfiger(InfrastConfiger&& rhs) noexcept = default;

        InfrastConfiger& operator=(const InfrastConfiger& rhs) = default;
        InfrastConfiger& operator=(InfrastConfiger&& rhs) noexcept = default;

        virtual bool parse(const json::value& json) override;
    };
}
