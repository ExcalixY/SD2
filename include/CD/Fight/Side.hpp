#pragma once

#include <array>
#include <cstddef>
#include <utility>

namespace CD {

enum class Side {
  PLAYER,
  CPU,
};

inline constexpr std::array SIDES = {Side::PLAYER, Side::CPU};

[[nodiscard]]
constexpr Side GetOpponent(const Side side) {
  return side == Side::PLAYER ? Side::CPU : Side::PLAYER;
}

[[nodiscard]]
constexpr size_t ToIndex(const Side side) {
  return static_cast<size_t>(std::to_underlying(side));
}

}
