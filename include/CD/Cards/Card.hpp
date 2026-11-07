#pragma once

#include <array>
#include <cstdint>

#include "CD/Cards/CardDef.hpp"
#include "CD/Cards/CardKind.hpp"

namespace CD {

struct Config;

using CardID = uint32_t;
inline constexpr CardID NO_CARD = 0;

struct Card {
  CardID id = NO_CARD;
  const CardDef* def = nullptr;
  Suit suit = Suit::CLUBS;
  std::array<int, TOKEN_COUNT> tokens = {};

  [[nodiscard]]
  int GetTokens(Token token) const;
  [[nodiscard]]
  int GetTokenCount() const;
};

[[nodiscard]]
int GetCardCost(const Card& card, const Config& config);
[[nodiscard]]
int GetCardStrength(const Card& card, const Config& config);

}
