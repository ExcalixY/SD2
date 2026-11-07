#pragma once

#include <array>
#include <string>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Deck/DeckStats.hpp"
#include "CD/Run/Upgrade.hpp"

namespace CD {

struct RunOptions {
  bool randomDiscard = false;
};

struct Run {
  std::string deckId;
  std::string deckName;
  std::vector<Card> cards;
  DeckStats deckStats;

  int gold = 0;
  int fight = 1;
  int wins = 0;
  bool wonRun = false;
  std::array<int, UPGRADE_COUNT> upgrades = {};
  RunOptions options;

  [[nodiscard]]
  int GetUpgrade(Upgrade upgrade) const;
};

}
