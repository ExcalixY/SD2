#include "CD/Fight/DiscardRisk.hpp"

#include <string_view>

namespace CD {

DiscardRisk GetDiscardRisk(const int cards_in_hand, const bool random_discard) {
  if (!random_discard) {
    return {};
  }

  const double chance = cards_in_hand <= 1 ? 1. : 1. / cards_in_hand;
  const std::string_view label = chance >= 1.     ? "Guaranteed"
                                 : chance >= 0.5  ? "Super High"
                                 : chance >= 0.35 ? "High"
                                 : chance >= 0.2  ? "Medium"
                                 : chance >= 0.1  ? "Low"
                                                  : "Super Low";
  return {.label = label, .chance = chance};
}

}
