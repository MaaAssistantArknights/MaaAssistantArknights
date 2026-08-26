#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "Common/AsstBattleDef.h"

namespace asst::infrast
{
struct ScoreOper
{
    std::unordered_set<std::string> skills;
    std::string operator_id;
    std::string face_hash;
    double mood_ratio = 0;
};

struct ScoreContext
{
    std::string facility;
    std::string product;
    int level = 1;
    int rarity = 0;
    int slots = 1;
    double mood_threshold = 0;
    int dormitory_capacity = 0;
    int dormitory_level_sum = 0;
    int gold_station_num = 0;
    int trading_station_num = 0;
    int power_station_num = 0;
    int virtual_power_station_num = 0;
    int total_station_level = 0;
    int workbench_num = 0;
    bool use_pinus_sylvestris = false;
    bool use_perception_information = false;
    bool use_worldly_plight = false;
    bool use_abyssal_hunter = false;
    std::unordered_set<std::string> selected_operator_ids;
};

struct ScoreResult
{
    std::vector<size_t> indices;
    double score = -1;
};

struct AbyssalHunterCandidate
{
    std::string operator_id;
    battle::Role role = battle::Role::Unknown;
};

inline constexpr std::string_view AbyssalHunterSkill = "abyssal_hunter";
const std::array<AbyssalHunterCandidate, 4>& get_abyssal_hunter_candidates();

bool is_abyssal_hunter(std::string_view operator_id);
void append_abyssal_hunter_candidates(std::vector<ScoreOper>& opers, const ScoreContext& context);

ScoreResult select_best_opers(const std::vector<ScoreOper>& opers, const ScoreContext& context);
} // namespace asst::infrast
