#include "CD/Deck/DeckStats.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <map>
#include <numeric>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardDef.hpp"
#include "CD/Cards/CardKind.hpp"
#include "CD/Cards/Catalog.hpp"
#include "CD/Config.hpp"

namespace CD {

namespace {

size_t IndexOf(const CardKind kind) {
  const auto it = std::ranges::find(BASIC_KINDS, kind);
  if (it == BASIC_KINDS.end()) {
    throw std::invalid_argument("DeckCounts only counts the basic card kinds.");
  }

  return static_cast<size_t>(it - BASIC_KINDS.begin());
}

}

int& DeckCounts::Of(const CardKind kind) {
  return basic[IndexOf(kind)];
}

int DeckCounts::Of(const CardKind kind) const {
  return basic[IndexOf(kind)];
}

int DeckCounts::GetTotal() const {
  return std::accumulate(basic.begin(), basic.end(), wild);
}

DeckStats GetDeckStats(
  std::span<const Card> cards, const Catalog& catalog, const Config& config, const std::optional<double> target
) {
  DeckStats stats = {.size = static_cast<int>(cards.size()), .target = target};
  std::map<const CardDef*, int> copies;

  for (const Card& card : cards) {
    if (card.def->isWild) {
      stats.counts.wild++;
    } else {
      stats.counts.Of(card.def->kind)++;
    }

    copies[card.def]++;
    stats.strength += GetCardStrength(card, config);
  }
  stats.unique = static_cast<int>(copies.size());

  const int catalog_size = static_cast<int>(catalog.GetCards().size());
  const int min_unique = (catalog_size + config.deck.uniqueDivisor - 1) / config.deck.uniqueDivisor;
  const int cap = static_cast<int>(std::floor(stats.size * config.deck.typeCap));
  const auto& basic = stats.counts.basic;

  int max_copies = 0;
  bool copies_ok = true;
  for (const auto& [def, count] : copies) {
    max_copies = std::max(max_copies, count);
    copies_ok = copies_ok && count <= def->maxCopies;
  }

  stats.checks = {
    {
      .rule = std::format("At least {} unique cards (1/{} of {})", min_unique, config.deck.uniqueDivisor, catalog_size),
      .ok = stats.unique >= min_unique,
      .value = std::format("{}", stats.unique),
    },
    {
      .rule = std::format("No type over {}% ({} cards)", std::lround(config.deck.typeCap * 100.), cap),
      .ok = std::ranges::all_of(basic, [cap](const int count) { return count <= cap; }),
      .value = std::format("{}/{}/{}/{}", basic[0], basic[1], basic[2], basic[3]),
    },
    {
      .rule = "Copies within maxCopies",
      .ok = copies_ok,
      .value = std::format("{}", max_copies),
    },
    {
      .rule = "Every type at least once",
      .ok = std::ranges::all_of(basic, [](const int count) { return count >= 1; }),
      .value = std::format("{}/4", std::ranges::count_if(basic, [](const int count) { return count >= 1; })),
    },
  };
  stats.valid = std::ranges::all_of(stats.checks, &DeckCheck::ok);

  if (target) {
    stats.checks.push_back({
      .rule = std::format("Strength near target {}", std::lround(*target)),
      .ok = std::abs(stats.strength - *target) <= *target * config.deck.strengthTolerance,
      .value = std::format("{}", stats.strength),
    });
  }

  return stats;
}

}
