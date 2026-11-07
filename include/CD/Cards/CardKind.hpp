#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace CD {

enum class CardKind {
  DAMAGE,
  MULT,
  BUFF,
  DEBUFF,
  WILD_ATTACK,
  WILD_SPELL,
};

inline constexpr std::array BASIC_KINDS = {CardKind::DAMAGE, CardKind::MULT, CardKind::BUFF, CardKind::DEBUFF};

enum class Suit {
  CLUBS,
  HEARTS,
  DIAMONDS,
  SPADES,
  BUFF,
  DEBUFF,
  WILD,
};

inline constexpr std::array PLAYING_SUITS = {Suit::CLUBS, Suit::HEARTS, Suit::DIAMONDS, Suit::SPADES};

enum class Token {
  YELLOW,
  PURPLE,
  GREEN,
  RED,
};

inline constexpr size_t TOKEN_COUNT = 4;
inline constexpr std::array TOKENS = {Token::YELLOW, Token::PURPLE, Token::GREEN, Token::RED};

[[nodiscard]]
std::string_view ToString(CardKind kind);
[[nodiscard]]
std::string_view ToString(Suit suit);
[[nodiscard]]
std::string_view ToString(Token token);

}
