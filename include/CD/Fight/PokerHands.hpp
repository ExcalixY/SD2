#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "CD/Cards/Card.hpp"
#include "CD/Fight/HandRank.hpp"

namespace CD {

class Catalog;
struct Config;

struct HandResult {
  HandRank rank = HandRank::HIGH_CARD;
  std::string_view name;
  int level = 1;
  int64_t damage = 0;
  int mult = 1;
};

[[nodiscard]]
std::optional<HandResult> EvaluateHand(
  std::span<const Card* const> played, const Catalog& catalog, const Config& config, int level
);

}
