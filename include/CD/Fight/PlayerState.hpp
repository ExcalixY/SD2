#pragma once

#include <cstdint>
#include <deque>
#include <queue>
#include <string>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Effects/Effect.hpp"
#include "Utils/circular_list.hpp"

namespace CD {

struct PlayerStats {
  int64_t bestHit = 0;
  int bestCombo = 1;
};

struct PlayerState {
  std::string name;
  bool isCpu = false;
  int64_t maxHp = 0;
  int64_t hp = 0;

  std::queue<CardID> deck;
  circular_list<CardID> hand;
  std::vector<CardID> played;
  std::queue<CardID> field;
  std::deque<Effect> effects;
  std::vector<CardID> discard;

  int handSize = 5;
  int fieldSize = 7;
  int areaSize = 3;
  int handLevel = 1;

  int combo = 1;
  int64_t lastHit = 0;
  PlayerStats stats;
};

}
