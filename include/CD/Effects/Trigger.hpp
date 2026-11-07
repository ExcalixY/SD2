#pragma once

#include <optional>
#include <string_view>

namespace CD {

enum class Trigger {
  PLAY,
  TURN_START,
  DRAW,
  AFTER_DRAW,
  DECK_REFILL,
  CARD_PLAYED,
  HAND_EVALUATED,
  FIELD_ENTER,
  FIELD_EJECT,
  CALCULATE,
  COMBO_CHECK,
  COMBO_UP,
  COMBO_BREAK,
  FINAL_SCORE,
  INCOMING_HIT,
  AFTER_HIT,
  HEAL,
  LETHAL,
  TURN_END,
  EFFECT_TICK,
  EFFECT_EXPIRED,
  EFFECT_DESTROYED,
  RANDOM_DISCARD,
};

[[nodiscard]]
std::string_view ToKey(Trigger trigger);
[[nodiscard]]
std::optional<Trigger> TriggerFromKey(std::string_view key);

}
