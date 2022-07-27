#pragma once
#include "AbstractConfiger.h"

#include <unordered_map>

#include "AsstTypes.h"
#include "AsstBattleDef.h"

namespace asst
{
    class BattleDataConfiger : public AbstractConfiger
    {
    public:
        virtual ~BattleDataConfiger() = default;
    protected:
        virtual bool parse(const json::value& json) override;

    private:
        std::unordered_map<std::string, BattleCharData> m_chars;
        std::unordered_map<std::string, std::vector<Point>> m_ranges;
    };
}
