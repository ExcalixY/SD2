#include "CD/Cards/CardKind.hpp"

#include <string_view>

namespace CD {

std::string_view ToString(const CardKind kind) {
  switch (kind) {
    case CardKind::DAMAGE:
      return "Damage";
    case CardKind::MULT:
      return "Mult";
    case CardKind::BUFF:
      return "Buff";
    case CardKind::DEBUFF:
      return "Debuff";
    case CardKind::WILD_ATTACK:
      return "Wild Attack";
    case CardKind::WILD_SPELL:
      return "Wild Spell";
  }

  return "?";
}

std::string_view ToString(const Suit suit) {
  switch (suit) {
    case Suit::CLUBS:
      return "Clubs";
    case Suit::HEARTS:
      return "Hearts";
    case Suit::DIAMONDS:
      return "Diamonds";
    case Suit::SPADES:
      return "Spades";
    case Suit::BUFF:
      return "Buff";
    case Suit::DEBUFF:
      return "Debuff";
    case Suit::WILD:
      return "Wild";
  }

  return "?";
}

std::string_view ToString(const Token token) {
  switch (token) {
    case Token::YELLOW:
      return "Yellow";
    case Token::PURPLE:
      return "Purple";
    case Token::GREEN:
      return "Green";
    case Token::RED:
      return "Red";
  }

  return "?";
}

}
