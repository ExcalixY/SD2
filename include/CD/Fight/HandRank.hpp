#pragma once

#include <cstddef>

namespace CD {

enum class HandRank {
  HIGH_CARD,
  PAIR,
  FLUSH,
  STRAIGHT,
  FLUSH_PAIR,
  THREE_OF_A_KIND,
  STRAIGHT_FLUSH,
  FLUSH_THREE,
};

inline constexpr size_t HAND_RANK_COUNT = 8;

}
