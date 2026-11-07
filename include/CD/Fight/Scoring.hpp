#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardKind.hpp"
#include "CD/Fight/PokerHands.hpp"

namespace CD {

struct Config;

struct ScoreBreakdown {
  std::optional<HandResult> hand;
  std::vector<CardID> cards;

  int64_t damageSum = 0;
  int multSum = 0;
  std::array<int, TOKEN_COUNT> tokens = {};

  double baseDamage = 0.;
  double baseMult = 1.;
  double damage = 0.;
  double mult = 1.;
  std::vector<std::string> notes;

  int64_t combo = 0;
  int64_t total = 0;
  double target = 0.;
};

[[nodiscard]]
ScoreBreakdown ScoreBase(
  std::span<const Card* const> cards, const std::optional<HandResult>& hand, const Config& config
);

void ScoreFinish(ScoreBreakdown& score, int combo_level, const Config& config);

}
