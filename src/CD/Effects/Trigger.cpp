#include "CD/Effects/Trigger.hpp"

#include <array>
#include <optional>
#include <string_view>
#include <utility>

namespace CD {

namespace {

constexpr std::array<std::pair<Trigger, std::string_view>, 23> KEYS = {{
  {Trigger::PLAY, "play"},
  {Trigger::TURN_START, "turnStart"},
  {Trigger::DRAW, "draw"},
  {Trigger::AFTER_DRAW, "afterDraw"},
  {Trigger::DECK_REFILL, "deckRefill"},
  {Trigger::CARD_PLAYED, "cardPlayed"},
  {Trigger::HAND_EVALUATED, "handEvaluated"},
  {Trigger::FIELD_ENTER, "fieldEnter"},
  {Trigger::FIELD_EJECT, "fieldEject"},
  {Trigger::CALCULATE, "calculate"},
  {Trigger::COMBO_CHECK, "comboCheck"},
  {Trigger::COMBO_UP, "comboUp"},
  {Trigger::COMBO_BREAK, "comboBreak"},
  {Trigger::FINAL_SCORE, "finalScore"},
  {Trigger::INCOMING_HIT, "incomingHit"},
  {Trigger::AFTER_HIT, "afterHit"},
  {Trigger::HEAL, "heal"},
  {Trigger::LETHAL, "lethal"},
  {Trigger::TURN_END, "turnEnd"},
  {Trigger::EFFECT_TICK, "effectTick"},
  {Trigger::EFFECT_EXPIRED, "effectExpired"},
  {Trigger::EFFECT_DESTROYED, "effectDestroyed"},
  {Trigger::RANDOM_DISCARD, "randomDiscard"},
}};

}

std::string_view ToKey(const Trigger trigger) {
  for (const auto& [value, key] : KEYS) {
    if (value == trigger) {
      return key;
    }
  }

  return "?";
}

std::optional<Trigger> TriggerFromKey(const std::string_view key) {
  for (const auto& [value, name] : KEYS) {
    if (name == key) {
      return value;
    }
  }

  return std::nullopt;
}

}
