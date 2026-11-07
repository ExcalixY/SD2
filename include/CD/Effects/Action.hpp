#pragma once

#include <optional>
#include <string_view>

namespace CD {

enum class Action {
  HEAL,
  MULTIPLY_MULT,
  MULTIPLY_DAMAGE,
  ADD_DAMAGE_PER_COMBO,
  KEEP_COMBO,
  DRAW_CARDS,
  ADD_COMBO,
  MULTIPLY_HIT,
  DEAL_DAMAGE,
  LIMIT_DRAW,
  DESTROY_FIELD,
  DESTROY_EFFECTS,
  SET_HP_PERCENT,
};

struct ActionSpec {
  Action action = Action::HEAL;
  std::optional<double> player;
  std::optional<double> opponent;
};

[[nodiscard]]
std::string_view ToKey(Action action);
[[nodiscard]]
std::optional<Action> ActionFromKey(std::string_view key);

}
