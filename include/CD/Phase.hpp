#pragma once

#include <string_view>

namespace CD {

enum class Phase {
  NO_RUN,
  PLAYER_TURN,
  CPU_TURN,
  FIGHT_WON,
  SHOP,
  RUN_LOST,
};

[[nodiscard]]
std::string_view ToString(Phase phase);

}
