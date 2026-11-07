#include "CD/Fight/PokerHands.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <set>
#include <span>
#include <utility>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardDef.hpp"
#include "CD/Cards/CardKind.hpp"
#include "CD/Cards/Catalog.hpp"
#include "CD/Config.hpp"
#include "CD/Fight/HandRank.hpp"
#include "CD/Format.hpp"

namespace CD {

namespace {

struct HandSlot {
  const CardDef* def = nullptr;
  Suit suit = Suit::CLUBS;
};

template <typename TKey>
size_t CountDistinct(std::span<const HandSlot> slots, TKey key) {
  std::set<decltype(key(slots.front()))> values;

  for (const HandSlot& slot : slots) {
    values.insert(key(slot));
  }

  return values.size();
}

HandRank RankOf(std::span<const HandSlot> slots) {
  const size_t count = slots.size();
  const size_t ids = CountDistinct(slots, [](const HandSlot& slot) { return slot.def; });
  const bool pair = count >= 2 && ids < count;

  if (count == 3) {
    const bool same_id = ids == 1;
    const bool same_suit = CountDistinct(slots, [](const HandSlot& slot) { return slot.suit; }) == 1;
    const bool same_kind = CountDistinct(slots, [](const HandSlot& slot) { return slot.def->kind; }) == 1;
    const bool straight = same_kind && ids == 3;

    if (same_id && same_suit) {
      return HandRank::FLUSH_THREE;
    }
    if (straight && same_suit) {
      return HandRank::STRAIGHT_FLUSH;
    }
    if (same_id) {
      return HandRank::THREE_OF_A_KIND;
    }
    if (same_suit && pair) {
      return HandRank::FLUSH_PAIR;
    }
    if (straight) {
      return HandRank::STRAIGHT;
    }
    if (same_suit) {
      return HandRank::FLUSH;
    }
  }

  return pair ? HandRank::PAIR : HandRank::HIGH_CARD;
}

void FindBest(
  std::span<const Card* const> played, std::span<const HandSlot> options, std::vector<HandSlot>& slots, HandRank& best
) {
  if (slots.size() == played.size()) {
    best = std::max(best, RankOf(slots));
    return;
  }

  const Card& card = *played[slots.size()];
  if (!card.def->isWild) {
    slots.push_back({.def = card.def, .suit = card.suit});
    FindBest(played, options, slots, best);
    slots.pop_back();
    return;
  }

  for (const HandSlot& option : options) {
    slots.push_back(option);
    FindBest(played, options, slots, best);
    slots.pop_back();
  }
}

}

std::optional<HandResult> EvaluateHand(
  std::span<const Card* const> played, const Catalog& catalog, const Config& config, const int level
) {
  if (played.empty()) {
    return std::nullopt;
  }

  std::vector<HandSlot> options;
  for (const CardKind kind : {CardKind::DAMAGE, CardKind::MULT}) {
    for (const CardDef* def : catalog.GetCardsOfKind(kind)) {
      for (const Suit suit : PLAYING_SUITS) {
        options.push_back({.def = def, .suit = suit});
      }
    }
  }

  HandRank best = HandRank::HIGH_CARD;
  std::vector<HandSlot> slots;
  FindBest(played, options, slots, best);

  const HandValues& values = config.hands[std::to_underlying(best)];
  return HandResult{
    .rank = best,
    .name = values.name,
    .level = level,
    .damage = RoundAmount(static_cast<double>(values.damage) * (1. + config.handLevelDamage * (level - 1))),
    .mult = values.mult + config.handLevelMult * (level - 1),
  };
}

}
