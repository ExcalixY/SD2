#include "CD/Phase.hpp"

#include <string_view>

namespace CD {

std::string_view ToString(const Phase phase) {
  switch (phase) {
    case Phase::NO_RUN:
      return "No run";
    case Phase::PLAYER_TURN:
      return "Player turn";
    case Phase::CPU_TURN:
      return "CPU turn";
    case Phase::FIGHT_WON:
      return "Fight won";
    case Phase::SHOP:
      return "Shop";
    case Phase::RUN_LOST:
      return "Run lost";
  }

  return "?";
}

}
