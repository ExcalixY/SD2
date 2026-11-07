#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

#include "CD/Fight/HandRank.hpp"
#include "CD/Run/Upgrade.hpp"

namespace CD {

struct HandValues {
  std::string_view name;
  int64_t damage = 0;
  int mult = 1;
};

struct TokenValues {
  double yellow = 200.;
  double purple = 0.5;
  double green = 3.;
  double red = 0.5;
  int cost = 2;
  int strength = 2;
};

struct ComboThreshold {
  int upToLevel = std::numeric_limits<int>::max();
  double growth = 0.1;
};

struct CardMeta {
  std::string_view id;
  int maxCopies = 3;
  int strength = 3;
};

struct DeckRules {
  double typeCap = 0.4;
  int uniqueDivisor = 3;
  double strengthTolerance = 0.1;
  int attempts = 60;
  int cpuMinSize = 12;
  double bossStrength = 1.15;
};

struct CpuGrowth {
  int every = 2;
  int max = 2;
};

struct GoldValues {
  int min = 5;
  int max = 10;
  int fastWinRounds = 10;
  int fastWinBonus = 2;
  int bossBonus = 5;
};

struct UpgradeValues {
  std::string_view name;
  int basePrice = 5;
};

struct ShopValues {
  int cardSlots = 3;
  int tokenPack = 4;
  double wildChance = 0.05;
  double tokenChance = 0.05;
  double upgradeGrowth = 1.2;
  std::array<UpgradeValues, UPGRADE_COUNT> upgrades = {{
    {.name = "Hand size +1", .basePrice = 8},
    {.name = "Field size +1", .basePrice = 6},
    {.name = "Buff/Debuff area +1", .basePrice = 7},
    {.name = "Max HP +20%", .basePrice = 5},
    {.name = "All poker hands +1 level", .basePrice = 6},
  }};
};

struct Config {
  int deckSize = 40;
  int handSize = 5;
  int fieldSize = 7;
  int areaSize = 3;
  int maxPlayedAttacks = 3;
  int maxPlayedSpells = 3;
  int maxWildsOnField = 2;

  int64_t playerHp = 100'000;
  double hpUpgrade = 0.2;
  int64_t cpuHp = 90'000;
  double cpuHpGrowth = 1.1;
  double bossHpMult = 1.2;
  int bossEvery = 5;
  CpuGrowth cpuHandGrowth = {.every = 2, .max = 2};
  CpuGrowth cpuFieldGrowth = {.every = 2, .max = 3};
  CpuGrowth cpuAreaGrowth = {.every = 3, .max = 2};
  int cpuLevelEvery = 2;

  int discardWalkMin = 20;
  int discardWalkMax = 50;

  TokenValues token;

  int comboMax = 14;
  std::vector<ComboThreshold> comboThresholds = {{.upToLevel = 6, .growth = 0.1}, {.growth = 0.2}};

  std::array<HandValues, HAND_RANK_COUNT> hands = {{
    {.name = "High Card", .damage = 50, .mult = 1},
    {.name = "Pair", .damage = 150, .mult = 2},
    {.name = "Flush", .damage = 250, .mult = 3},
    {.name = "Straight", .damage = 300, .mult = 3},
    {.name = "Flush Pair", .damage = 400, .mult = 4},
    {.name = "Three of a Kind", .damage = 500, .mult = 5},
    {.name = "Straight Flush", .damage = 700, .mult = 6},
    {.name = "Flush Three", .damage = 1000, .mult = 8},
  }};
  double handLevelDamage = 0.5;
  int handLevelMult = 1;

  std::vector<CardMeta> cardMeta = {
    {.id = "small_blind", .maxCopies = 6, .strength = 2},    {.id = "big_blind", .maxCopies = 5, .strength = 3},
    {.id = "straddle", .maxCopies = 3, .strength = 5},       {.id = "two_to_one", .maxCopies = 6, .strength = 2},
    {.id = "five_to_one", .maxCopies = 5, .strength = 3},    {.id = "ten_to_one", .maxCopies = 3, .strength = 5},
    {.id = "refund", .maxCopies = 3, .strength = 2},         {.id = "double_down", .maxCopies = 3, .strength = 3},
    {.id = "lucky_number_7", .maxCopies = 3, .strength = 3}, {.id = "hit_me", .maxCopies = 3, .strength = 3},
    {.id = "let_it_ride", .maxCopies = 3, .strength = 3},    {.id = "no_action", .maxCopies = 3, .strength = 4},
    {.id = "rake", .maxCopies = 3, .strength = 3},           {.id = "cold_streak", .maxCopies = 3, .strength = 3},
    {.id = "misdeal", .maxCopies = 3, .strength = 2},        {.id = "burn_it_up", .maxCopies = 3, .strength = 4},
    {.id = "no_dice", .maxCopies = 3, .strength = 3},        {.id = "jackpot", .maxCopies = 2, .strength = 6},
    {.id = "infinity", .maxCopies = 2, .strength = 6},       {.id = "saving_grace", .maxCopies = 2, .strength = 6},
    {.id = "black_hole", .maxCopies = 2, .strength = 6},
  };
  CardMeta defaultMeta = {.id = "", .maxCopies = 3, .strength = 3};
  int cardCost = 3;
  int wildCost = 7;

  DeckRules deck;
  GoldValues gold;
  ShopValues shop;
};

}
