#pragma once

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardDef.hpp"
#include "CD/Cards/CardKind.hpp"

namespace CD {

class Random;

class CardFactory final {
public:
  [[nodiscard]]
  Card Make(const CardDef& def, Random& random);
  [[nodiscard]]
  Card Make(const CardDef& def, Suit suit);

  static void AddToken(Card& card, Token token);
  static Token AddRandomToken(Card& card, Random& random);

private:
  CardID m_NextId = NO_CARD + 1;
};

}
