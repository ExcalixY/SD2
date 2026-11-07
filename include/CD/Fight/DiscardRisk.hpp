#pragma once

#include <string_view>

namespace CD {

struct DiscardRisk {
  std::string_view label = "None";
  double chance = 0.;
};

[[nodiscard]]
DiscardRisk GetDiscardRisk(int cards_in_hand, bool random_discard);

}
