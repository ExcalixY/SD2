#include "CD/Cards/Card.hpp"

#include <numeric>
#include <utility>

#include "CD/Cards/CardKind.hpp"
#include "CD/Config.hpp"

namespace CD {

int Card::GetTokens(const Token token) const {
  return tokens[std::to_underlying(token)];
}

int Card::GetTokenCount() const {
  return std::accumulate(tokens.begin(), tokens.end(), 0);
}

int GetCardCost(const Card& card, const Config& config) {
  return card.def->cost + card.GetTokenCount() * config.token.cost;
}

int GetCardStrength(const Card& card, const Config& config) {
  return card.def->strength + card.GetTokenCount() * config.token.strength;
}

}
