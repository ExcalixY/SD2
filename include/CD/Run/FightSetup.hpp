#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "CD/Cards/Card.hpp"

namespace CD {

class CardFactory;
class Catalog;
class Random;
struct Config;
struct Run;

struct PlayerSetup {
  std::string name;
  bool isCpu = false;
  std::vector<Card> cards;
  int64_t maxHp = 0;
  int handSize = 5;
  int fieldSize = 7;
  int areaSize = 3;
  int handLevel = 1;
};

struct FightSetup {
  int number = 1;
  bool boss = false;
  bool randomDiscard = false;
  PlayerSetup player;
  PlayerSetup cpu;
};

[[nodiscard]]
bool IsBossFight(int fight, const Config& config);

[[nodiscard]]
std::optional<FightSetup> MakeFightSetup(
  const Run& run, const Catalog& catalog, const Config& config, CardFactory& factory, Random& random
);

}
