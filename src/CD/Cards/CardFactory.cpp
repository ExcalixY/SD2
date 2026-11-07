#include "CD/Cards/CardFactory.hpp"

#include <utility>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardDef.hpp"
#include "CD/Cards/CardKind.hpp"
#include "CD/Random.hpp"

namespace CD {

Card CardFactory::Make(const CardDef& def, Random& random) {
  const Suit suit = def.isWild                   ? Suit::WILD
                    : def.isAttack               ? random.Pick(PLAYING_SUITS)
                    : def.kind == CardKind::BUFF ? Suit::BUFF
                                                 : Suit::DEBUFF;
  return Make(def, suit);
}

Card CardFactory::Make(const CardDef& def, const Suit suit) {
  return {.id = m_NextId++, .def = &def, .suit = suit};
}

void CardFactory::AddToken(Card& card, const Token token) {
  card.tokens[std::to_underlying(token)]++;
}

Token CardFactory::AddRandomToken(Card& card, Random& random) {
  const Token token = random.Pick(TOKENS);
  AddToken(card, token);
  return token;
}

}
