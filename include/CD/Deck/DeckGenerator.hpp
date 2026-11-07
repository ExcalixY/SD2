#pragma once

#include <array>
#include <optional>
#include <span>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardKind.hpp"
#include "CD/Deck/DeckStats.hpp"

namespace CD {

class CardFactory;
class Catalog;
class Random;
struct Config;
struct DeckBalance;

struct GeneratedDeck {
  std::vector<Card> cards;
  DeckStats stats;
};

[[nodiscard]]
DeckCounts GetDeckCounts(
  const DeckBalance& balance, int size, const Catalog& catalog, const Config& config, Random& random
);

[[nodiscard]]
DeckCounts ScaleCounts(const std::array<int, BASIC_KINDS.size()>& raw, int size, int wild = 0);

[[nodiscard]]
std::optional<GeneratedDeck> GenerateDeck(
  const DeckCounts& counts,
  const Catalog& catalog,
  const Config& config,
  CardFactory& factory,
  Random& random,
  std::optional<double> target = {}
);

[[nodiscard]]
std::optional<GeneratedDeck> GenerateCpuDeck(
  std::span<const Card> player_cards,
  bool boss,
  const Catalog& catalog,
  const Config& config,
  CardFactory& factory,
  Random& random
);

}
