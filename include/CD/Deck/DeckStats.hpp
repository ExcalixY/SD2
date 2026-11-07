#pragma once

#include <array>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "CD/Cards/Card.hpp"
#include "CD/Cards/CardKind.hpp"

namespace CD {

class Catalog;
struct Config;

struct DeckCounts {
  std::array<int, BASIC_KINDS.size()> basic = {};
  int wild = 0;

  [[nodiscard]]
  int& Of(CardKind kind);
  [[nodiscard]]
  int Of(CardKind kind) const;
  [[nodiscard]]
  int GetTotal() const;
};

struct DeckCheck {
  std::string rule;
  bool ok = false;
  std::string value;
};

struct DeckStats {
  int size = 0;
  DeckCounts counts;
  int unique = 0;
  int strength = 0;
  std::optional<double> target;
  std::vector<DeckCheck> checks;
  bool valid = false;
};

[[nodiscard]]
DeckStats GetDeckStats(
  std::span<const Card> cards, const Catalog& catalog, const Config& config, std::optional<double> target = {}
);

}
