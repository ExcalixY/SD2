#include "CD/Deck/DeckGenerator.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <map>
#include <optional>
#include <span>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardDef.hpp"
#include "CD/Cards/CardFactory.hpp"
#include "CD/Cards/CardKind.hpp"
#include "CD/Cards/Catalog.hpp"
#include "CD/Cards/DeckDef.hpp"
#include "CD/Config.hpp"
#include "CD/Deck/DeckStats.hpp"
#include "CD/Random.hpp"

namespace CD {

namespace {

bool Deal(
  std::span<const CardDef* const> defs,
  const int count,
  std::map<const CardDef*, int>& copies,
  std::vector<Card>& cards,
  CardFactory& factory,
  Random& random
) {
  for (int i = 0; i < count; i++) {
    std::vector<const CardDef*> open;
    for (const CardDef* def : defs) {
      if (copies[def] < def->maxCopies) {
        open.push_back(def);
      }
    }

    if (open.empty()) {
      return false;
    }

    const CardDef* def = random.Pick(open);
    copies[def]++;
    cards.push_back(factory.Make(*def, random));
  }

  return true;
}

}

DeckCounts GetDeckCounts(
  const DeckBalance& balance, const int size, const Catalog& catalog, const Config& config, Random& random
) {
  if (!balance.random) {
    const std::array raw = {balance.damage, balance.mult, balance.buff, balance.debuff};
    const int wild = balance.wildDamage + balance.wildMagic;
    return ScaleCounts(raw, size - wild, wild);
  }

  const auto cap = [&](const CardKind kind) {
    int room = 0;
    for (const CardDef* def : catalog.GetCardsOfKind(kind)) {
      room += def->maxCopies;
    }
    return std::min(static_cast<int>(std::floor(size * config.deck.typeCap)), room);
  };

  DeckCounts counts = {.basic = {1, 1, 1, 1}, .wild = balance.allowWildRandom ? random.Int(1, 4) : 0};
  for (int left = size - counts.GetTotal(); left > 0; left--) {
    std::vector<CardKind> open;
    for (const CardKind kind : BASIC_KINDS) {
      if (counts.Of(kind) < cap(kind)) {
        open.push_back(kind);
      }
    }

    if (open.empty()) {
      break;
    }
    counts.Of(random.Pick(open))++;
  }

  return counts;
}

DeckCounts ScaleCounts(const std::array<int, BASIC_KINDS.size()>& raw, const int size, const int wild) {
  int total = 0;
  for (const int count : raw) {
    total += count;
  }

  DeckCounts counts = {.wild = wild};
  int used = 0;
  for (size_t i = 0; i < raw.size(); i++) {
    const int scaled = total > 0 ? raw[i] * size / total : size / static_cast<int>(raw.size());
    counts.basic[i] = std::max(1, scaled);
    used += counts.basic[i];
  }

  for (size_t i = 0; used < size; i = (i + 1) % raw.size()) {
    counts.basic[i]++;
    used++;
  }

  return counts;
}

std::optional<GeneratedDeck> GenerateDeck(
  const DeckCounts& counts,
  const Catalog& catalog,
  const Config& config,
  CardFactory& factory,
  Random& random,
  const std::optional<double> target
) {
  const std::vector<const CardDef*> basic = catalog.GetBasicCards();
  const std::vector<const CardDef*> wilds = catalog.GetWildCards();

  double average = 0.;
  for (const CardDef* def : basic) {
    average += def->strength;
  }
  average /= static_cast<double>(std::max<size_t>(1, basic.size()));
  const double goal = target.value_or(average * counts.GetTotal());

  std::optional<GeneratedDeck> best;
  for (int attempt = 0; attempt < config.deck.attempts; attempt++) {
    std::vector<Card> cards;
    std::map<const CardDef*, int> copies;

    bool dealt = true;
    for (const CardKind kind : BASIC_KINDS) {
      dealt = dealt && Deal(catalog.GetCardsOfKind(kind), counts.Of(kind), copies, cards, factory, random);
    }
    dealt = dealt && Deal(wilds, counts.wild, copies, cards, factory, random);
    if (!dealt) {
      continue;
    }

    DeckStats stats = GetDeckStats(cards, catalog, config, goal);
    if (!stats.valid) {
      continue;
    }

    if (!best || std::abs(stats.strength - goal) < std::abs(best->stats.strength - goal)) {
      best = GeneratedDeck{.cards = std::move(cards), .stats = std::move(stats)};
    }
  }

  return best;
}

std::optional<GeneratedDeck> GenerateCpuDeck(
  std::span<const Card> player_cards,
  const bool boss,
  const Catalog& catalog,
  const Config& config,
  CardFactory& factory,
  Random& random
) {
  const int size = std::max(config.deck.cpuMinSize, static_cast<int>(player_cards.size()));
  const DeckCounts counts = ScaleCounts({1, 1, 1, 1}, size);

  double base = 0.;
  for (const Card& card : player_cards) {
    if (!card.def->isWild) {
      base += card.def->strength;
    }
  }
  base = base * size / static_cast<double>(std::max<size_t>(1, player_cards.size()));

  std::optional<GeneratedDeck> deck =
    GenerateDeck(counts, catalog, config, factory, random, base * (boss ? config.deck.bossStrength : 1.));
  if (!deck) {
    return std::nullopt;
  }

  std::vector<Card*> attacks;
  for (Card& card : deck->cards) {
    if (card.def->isAttack) {
      attacks.push_back(&card);
    }
  }
  random.Shuffle(attacks);

  if (!attacks.empty()) {
    size_t next = 0;
    for (const Card& card : player_cards) {
      const int tokens = card.GetTokenCount();
      if (tokens == 0) {
        continue;
      }

      Card& target = *attacks[next++ % attacks.size()];
      for (int i = 0; i < tokens; i++) {
        CardFactory::AddRandomToken(target, random);
      }
    }
  }

  if (boss) {
    deck->cards.push_back(factory.Make(*random.Pick(catalog.GetWildCards()), random));
  }

  deck->stats = GetDeckStats(deck->cards, catalog, config, deck->stats.target);
  return deck;
}

}
