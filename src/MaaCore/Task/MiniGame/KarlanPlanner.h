#pragma once

#include <array>
#include <bitset>
#include <optional>
#include <vector>

namespace asst::karlan
{
enum class Resource
{
    Mine,
    Wood,
    Gear,
    Oil,
    Metal,
    Done
};

struct Rules
{
    std::array<Resource, 3> resources;
    int initial_yield;
    int lap_gain = 0;
    bool double_yield = false;
    bool locks = false;
    bool chests = false;
};

const Rules& rules(int stage);

struct Tile
{
    Resource resource;
    bool locked = false;
    bool boosted = false;
};

struct Requirement
{
    Resource resource;
    int delivered;
    int target;
};

using Orders = std::array<std::vector<Requirement>, 3>;

struct Action
{
    int card;
    bool collect;
    bool operator==(const Action&) const = default;
};

struct Board
{
    std::array<Tile, 10> tiles {};
    std::bitset<10> chests;
    std::vector<int> cards;
    std::bitset<6> combos;
    int position = 0;
    int production = 0;
    Orders orders;
};

std::optional<Action> choose(
    const Board& board,
    const Rules& rules,
    bool can_collect,
    int previous,
    const std::vector<Action>& unavailable);
}
