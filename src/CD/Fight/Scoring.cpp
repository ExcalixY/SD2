#include "CD/Fight/Scoring.hpp"

#include <algorithm>
#include <optional>
#include <span>
#include <utility>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardKind.hpp"
#include "CD/Config.hpp"
#include "CD/Fight/Combo.hpp"
#include "CD/Fight/PokerHands.hpp"
#include "CD/Format.hpp"

namespace CD {

ScoreBreakdown ScoreBase(
  std::span<const Card* const> cards, const std::optional<HandResult>& hand, const Config& config
) {
  ScoreBreakdown score;
  score.hand = hand;

  for (const Card* card : cards) {
    score.cards.push_back(card->id);
    score.damageSum += card->def->damage;
    score.multSum += card->def->mult;
    for (const Token token : TOKENS) {
      score.tokens[std::to_underlying(token)] += card->GetTokens(token);
    }
  }

  const auto tokens = [&score](const Token token) {
    return static_cast<double>(score.tokens[std::to_underlying(token)]);
  };
  const double hand_damage = hand ? static_cast<double>(hand->damage) : 0.;
  const double hand_mult = hand ? static_cast<double>(hand->mult) : 0.;

  score.baseDamage =
    (static_cast<double>(score.damageSum) + tokens(Token::YELLOW) * config.token.yellow + hand_damage) *
    (1. + tokens(Token::PURPLE) * config.token.purple);
  score.baseMult = std::max(
    1.,
    (static_cast<double>(score.multSum) + tokens(Token::GREEN) * config.token.green + hand_mult) *
      (1. + tokens(Token::RED) * config.token.red)
  );
  score.damage = score.baseDamage;
  score.mult = score.baseMult;
  return score;
}

void ScoreFinish(ScoreBreakdown& score, const int combo_level, const Config& config) {
  score.mult = std::max(1., score.mult);
  score.combo = GetComboDamage(combo_level);
  score.total = RoundAmount(score.damage * score.mult + static_cast<double>(score.combo));
  score.target = GetComboTarget(combo_level, config);
}

}
